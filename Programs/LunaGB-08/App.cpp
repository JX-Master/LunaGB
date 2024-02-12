#include "App.hpp"
#include <Luna/Runtime/Log.hpp>
#include <Luna/ImGui/ImGui.hpp>
#include <Luna/Window/FileDialog.hpp>
#include <Luna/Window/MessageBox.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Runtime/Time.hpp>
#include <Luna/ShaderCompiler/ShaderCompiler.hpp>
#include <Luna/RHI/ShaderCompileHelper.hpp>
#include <Luna/RHI/Utility.hpp>

RV App::init()
{
    lutry
    {
        is_exiting = false;
        // In order to see LunaSDK logs.
        set_log_to_platform_enabled(true);
        rhi_device = RHI::get_main_device();
        u32 num_queues = rhi_device->get_num_command_queues();
        rhi_queue_index = U32_MAX;
        for (u32 i = 0; i < num_queues; ++i)
        {
            auto desc = rhi_device->get_command_queue_desc(i);
            if (desc.type == RHI::CommandQueueType::graphics && rhi_queue_index == U32_MAX)
            {
                rhi_queue_index = i;
                break;
            }
        }
        if(rhi_queue_index == U32_MAX)
        {
            return set_error(BasicError::not_supported(), "No suitable GPU present queue found.");
        }
        // Create window and RHI resources.
        luset(window, Window::new_window("LunaGB", Window::WindowDisplaySettings::as_windowed(Window::DEFAULT_POS, Window::DEFAULT_POS, 1000, 1000), Window::WindowCreationFlag::resizable));
        luset(swap_chain, rhi_device->new_swap_chain(rhi_queue_index, window, RHI::SwapChainDesc({0, 0, 2, RHI::Format::bgra8_unorm, true})));
        luset(cmdbuf, rhi_device->new_command_buffer(rhi_queue_index));
        // Register close event.
        window->get_close_event().add_handler([](Window::IWindow* window) { window->close(); });
        // Register resize event.
        window->get_resize_event().add_handler([this](Window::IWindow* window, u32 width, u32 height)
        {
            auto _ = swap_chain->reset({width, height, 2, RHI::Format::unknown, true});
        });
        ImGuiUtils::set_active_window(window);
        last_frame_ticks = get_ticks();
        luexp(init_render_resources());
    }
    lucatchret;
    return ok;
}
struct EmulatorDisplayUB
{
    Float4x4U projection_matrix;
};

struct EmulatorDisplayVertex
{
    Float2U pos;
    Float2U uv;
};
RV App::init_render_resources()
{
    lutry
    {
        luset(emulator_display_tex, rhi_device->new_texture(RHI::MemoryType::local, 
            RHI::TextureDesc::tex2d(RHI::Format::rgba8_unorm, RHI::TextureUsageFlag::copy_dest | RHI::TextureUsageFlag::read_texture,
                PPU_XRES, PPU_YRES, 1, 1)));
            u32 ub_align = rhi_device->check_feature(RHI::DeviceFeature::uniform_buffer_data_alignment).uniform_buffer_data_alignment;
        luset(emulator_display_ub, rhi_device->new_buffer(RHI::MemoryType::upload, RHI::BufferDesc(RHI::BufferUsageFlag::uniform_buffer,
            align_upper(sizeof(EmulatorDisplayUB), ub_align))));
        luset(emulator_display_vb, rhi_device->new_buffer(RHI::MemoryType::upload, RHI::BufferDesc(RHI::BufferUsageFlag::vertex_buffer,
            sizeof(EmulatorDisplayVertex) * 4)));
        luset(emulator_display_ib, rhi_device->new_buffer(RHI::MemoryType::upload, RHI::BufferDesc(RHI::BufferUsageFlag::index_buffer,
            sizeof(u16) * 6)));
        u16* index_data;
        luexp(emulator_display_ib->map(0, 0, (void**)&index_data));
        index_data[0] = 0;
        index_data[1] = 1;
        index_data[2] = 2;
        index_data[3] = 0;
        index_data[4] = 2;
        index_data[5] = 3;
        emulator_display_ib->unmap(0, sizeof(u16) * 6);
        luset(emulator_display_dlayout, rhi_device->new_descriptor_set_layout(RHI::DescriptorSetLayoutDesc(
            { RHI::DescriptorSetLayoutBinding::uniform_buffer_view(0, 1, RHI::ShaderVisibilityFlag::vertex),
              RHI::DescriptorSetLayoutBinding::read_texture_view(RHI::TextureViewType::tex2d, 1, 1, RHI::ShaderVisibilityFlag::pixel),
              RHI::DescriptorSetLayoutBinding::sampler(2, 1, RHI::ShaderVisibilityFlag::pixel) })));
        luset(emulator_display_desc_set, rhi_device->new_descriptor_set(RHI::DescriptorSetDesc(emulator_display_dlayout)));
        RHI::IDescriptorSetLayout* dlayout = emulator_display_dlayout;
        luset(emulator_display_playout, rhi_device->new_pipeline_layout(RHI::PipelineLayoutDesc({ &dlayout, 1 },
            RHI::PipelineLayoutFlag::allow_input_assembler_input_layout)));
        const c8 vs[] = R"(
cbuffer ub_b0 : register(b0) 
{
    float4x4 projection_matrix; 
};
Texture2D texture0 : register(t1);
SamplerState sampler0 : register(s2);
struct VS_INPUT
{
    [[vk::location(0)]]
    float2 pos : POSITION;
    [[vk::location(1)]]
    float2 uv  : TEXCOORD0;
};
struct PS_INPUT
{
    [[vk::location(0)]]
    float4 pos : SV_POSITION;
    [[vk::location(1)]]
    float2 uv  : TEXCOORD0;
};
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
	output.pos = mul( projection_matrix, float4(input.pos.xy, 0.f, 1.f));
	output.uv  = input.uv;
	return output;
})";
            const c8 ps[] = R"(
struct PS_INPUT
{
    [[vk::location(0)]]
    float4 pos : SV_POSITION;
    [[vk::location(1)]]
    float2 uv  : TEXCOORD0;
};
cbuffer ub_b0 : register(b0) 
{
    float4x4 projection_matrix; 
};
Texture2D texture0 : register(t1);
SamplerState sampler0 : register(s2);
[[vk::location(0)]]
float4 main(PS_INPUT input) : SV_Target
{
    return texture0.Sample(sampler0, input.uv); 
}
)";
        auto compiler_vs = ShaderCompiler::new_compiler();
        compiler_vs->set_target_format(RHI::get_current_platform_shader_target_format());
        compiler_vs->set_source({ vs , sizeof(vs) });
        compiler_vs->set_shader_type(ShaderCompiler::ShaderType::vertex);
        compiler_vs->set_shader_model(6, 0);
        luexp(compiler_vs->compile());
        auto vs_data = compiler_vs->get_output();

        auto compiler_ps = ShaderCompiler::new_compiler();
        compiler_ps->set_target_format(RHI::get_current_platform_shader_target_format());
        compiler_ps->set_source({ ps , sizeof(ps) });
        compiler_ps->set_shader_type(ShaderCompiler::ShaderType::pixel);
        compiler_ps->set_shader_model(6, 0);
        luexp(compiler_ps->compile());
        auto ps_data = compiler_ps->get_output();
        
        RHI::GraphicsPipelineStateDesc pso;
        pso.primitive_topology = RHI::PrimitiveTopology::triangle_list;
        pso.rasterizer_state = RHI::RasterizerDesc(RHI::FillMode::solid, RHI::CullMode::none);
        pso.depth_stencil_state = RHI::DepthStencilDesc(false, false, RHI::CompareFunction::always, false, 0x00, 0x00, RHI::DepthStencilOpDesc(), RHI::DepthStencilOpDesc());
        RHI::InputBindingDesc input_bindings[] = {
            RHI::InputBindingDesc(0, sizeof(EmulatorDisplayVertex), RHI::InputRate::per_vertex)
        };
        RHI::InputAttributeDesc input_attributes[] = {
            RHI::InputAttributeDesc("POSITION", 0, 0, 0, 0, RHI::Format::rg32_float),
            RHI::InputAttributeDesc("TEXCOORD", 0, 1, 0, 8, RHI::Format::rg32_float)
        };
        pso.input_layout.bindings = { input_bindings, 1 };
        pso.input_layout.attributes = { input_attributes , 2 };
        pso.vs = vs_data;
        pso.ps = ps_data;
        pso.pipeline_layout = emulator_display_playout;
        pso.num_color_attachments = 1;
        pso.color_formats[0] = RHI::Format::bgra8_unorm;
        luset(emulator_display_pso, rhi_device->new_graphics_pipeline_state(pso));
        luexp(emulator_display_desc_set->update_descriptors({
            RHI::WriteDescriptorSet::uniform_buffer_view(0, RHI::BufferViewDesc::uniform_buffer(emulator_display_ub, 0, align_upper(sizeof(EmulatorDisplayUB), ub_align))),
            RHI::WriteDescriptorSet::read_texture_view(1, RHI::TextureViewDesc::tex2d(emulator_display_tex)),
            RHI::WriteDescriptorSet::sampler(2, RHI::SamplerDesc(RHI::Filter::nearest, RHI::Filter::nearest, RHI::Filter::nearest, RHI::TextureAddressMode::clamp, RHI::TextureAddressMode::clamp, RHI::TextureAddressMode::clamp))
                }));
    }
    lucatchret;
    return ok;
}
RV App::update()
{
    lutry
    {
        // Update window events.
        Window::poll_events();
        // Exit the program if the window is closed.
        if (window->is_closed())
        {
            is_exiting = true;
            return ok;
        }
        u64 ticks = get_ticks();
        u64 delta_ticks = ticks - last_frame_ticks;
        f64 delta_time = (f64)delta_ticks / get_ticks_per_second();
        delta_time = min(delta_time, 0.125);
        if(emulator)
        {
            emulator->update(delta_time);
        }
        last_frame_ticks = ticks;
        // Draw GUI.
        draw_gui();

        // Upload emulator screen pixels.
        if(emulator)
        {
            u8* src = emulator->ppu.pixels + ((emulator->ppu.current_back_buffer + 1) % 2) * PPU_XRES * PPU_YRES * 4;
            // Copy display data to texture for rendering.
            luexp(RHI::copy_resource_data(g_app->cmdbuf, {
                RHI::CopyResourceData::write_texture(emulator_display_tex, {0, 0}, 0, 0, 0, src, PPU_XRES * 4, PPU_XRES * PPU_YRES * 4, PPU_XRES, PPU_YRES, 1)
            }));
        }
        // Clear back buffer.
        lulet(back_buffer, swap_chain->get_current_back_buffer());
        Float4U clear_color = { 0.3f, 0.3f, 0.3f, 1.3f };
        RHI::RenderPassDesc render_pass;
        render_pass.color_attachments[0] = RHI::ColorAttachment(back_buffer, RHI::LoadOp::clear, RHI::StoreOp::store, clear_color);
        cmdbuf->begin_render_pass(render_pass);
        cmdbuf->end_render_pass();
        // Draw emulator screen.
        luexp(draw_emulator_screen(back_buffer));
        // Render GUI.
        luexp(ImGuiUtils::render_draw_data(ImGui::GetDrawData(), cmdbuf, back_buffer));
        cmdbuf->resource_barrier({}, {
            {back_buffer, RHI::TEXTURE_BARRIER_ALL_SUBRESOURCES, RHI::TextureStateFlag::automatic, RHI::TextureStateFlag::present, RHI::ResourceBarrierFlag::none}
            });
        // Submit render commands and present the back buffer.
        luexp(cmdbuf->submit({}, {}, true));
        cmdbuf->wait();
        luexp(cmdbuf->reset());
        luexp(swap_chain->present());
    }
    lucatchret;
    return ok;
}
RV App::draw_emulator_screen(RHI::ITexture* back_buffer)
{
    lutry
    {
        if(emulator.get())
        {
            auto window_sz = window->get_framebuffer_size();
            f32 window_width = window_sz.x;
            f32 window_height = window_sz.y;
            EmulatorDisplayVertex* vb_mapped;
            luexp(emulator_display_vb->map(0, 0, (void**)&vb_mapped));
            f32 display_width = PPU_XRES * 4;
            f32 display_height = PPU_YRES * 4;
            vb_mapped[0].pos = Float2U((window_width - display_width) / 2.0f, (window_height - display_height) / 2.0f);
            vb_mapped[0].uv = Float2U(0.0f, 0.0f);
            vb_mapped[1].pos = Float2U((window_width + display_width) / 2.0f, (window_height - display_height) / 2.0f);
            vb_mapped[1].uv = Float2U(1.0f, 0.0f);
            vb_mapped[2].pos = Float2U((window_width + display_width) / 2.0f, (window_height + display_height) / 2.0f);
            vb_mapped[2].uv = Float2U(1.0f, 1.0f);
            vb_mapped[3].pos = Float2U((window_width - display_width) / 2.0f, (window_height + display_height) / 2.0f);
            vb_mapped[3].uv = Float2U(0.0f, 1.0f);
            emulator_display_vb->unmap(0, sizeof(EmulatorDisplayVertex) * 4);
            EmulatorDisplayUB* ub_mapped;
            luexp(emulator_display_ub->map(0, 0, (void**)&ub_mapped));
            ub_mapped->projection_matrix = {
                { 2.0f / window_width, 0.0f,				    0.0f,       0.0f },
                { 0.0f,		            2.0f / -window_height,  0.0f,       0.0f },
                { 0.0f,		            0.0f,				    0.5f,       0.0f },
                { -1.0f,	            1.0f,                   0.5f,       1.0f },
            };
            emulator_display_ub->unmap(0, sizeof(EmulatorDisplayUB));
            cmdbuf->resource_barrier({
                RHI::BufferBarrier(emulator_display_ub, RHI::BufferStateFlag::automatic, RHI::BufferStateFlag::uniform_buffer_vs),
                RHI::BufferBarrier(emulator_display_vb, RHI::BufferStateFlag::automatic, RHI::BufferStateFlag::vertex_buffer),
                RHI::BufferBarrier(emulator_display_ib, RHI::BufferStateFlag::automatic, RHI::BufferStateFlag::index_buffer)
                }, {
                RHI::TextureBarrier(emulator_display_tex, RHI::SubresourceIndex(0, 0), RHI::TextureStateFlag::automatic, RHI::TextureStateFlag::shader_read_ps),
                RHI::TextureBarrier(back_buffer, RHI::SubresourceIndex(0, 0), RHI::TextureStateFlag::automatic, RHI::TextureStateFlag::color_attachment_write)
                });
            RHI::RenderPassDesc render_pass;
            render_pass.color_attachments[0] = RHI::ColorAttachment(back_buffer, RHI::LoadOp::load, RHI::StoreOp::store);
            cmdbuf->begin_render_pass(render_pass);
            cmdbuf->set_graphics_pipeline_layout(emulator_display_playout);
            cmdbuf->set_graphics_pipeline_state(emulator_display_pso);
            cmdbuf->set_graphics_descriptor_set(0, emulator_display_desc_set);
            cmdbuf->set_vertex_buffers(0, { RHI::VertexBufferView(emulator_display_vb, 0, sizeof(EmulatorDisplayVertex) * 4, sizeof(EmulatorDisplayVertex)) });
            cmdbuf->set_index_buffer(RHI::IndexBufferView(emulator_display_ib, 0, sizeof(u16) * 6, RHI::Format::r16_uint));
            cmdbuf->set_viewport(RHI::Viewport(0.0f, 0.0f, window_sz.x, window_sz.y, 0.0f, 1.0f));
            cmdbuf->set_scissor_rect(RectI(0, 0, window_sz.x, window_sz.y));
            cmdbuf->draw_indexed(6, 0, 0);
            cmdbuf->end_render_pass();
        }
    }
    lucatchret;
    return ok;
}
void App::draw_gui()
{
    // Begin GUI.
    ImGuiUtils::update_io();
    ImGui::NewFrame();

    draw_main_menu_bar();

    if(debug_window.show)
    {
        debug_window.gui();
    }

    // End GUI.
    ImGui::Render();
}
void App::draw_main_menu_bar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Open"))
            {
                open_cartridge();
            }
            if(ImGui::MenuItem("Open without playing"))
            {
                open_cartridge();
                if(emulator)
                {
                    emulator->paused = true;
                }
            }
            if(ImGui::MenuItem("Close"))
            {
                close_cartridge();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Play"))
        {
            if(ImGui::MenuItem("Play"))
            {
                if(emulator)
                {
                    emulator->paused = false;
                }
            }
            if(ImGui::MenuItem("Pause"))
            {
                if(emulator)
                {
                    emulator->paused = true;
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug"))
        {
            if(ImGui::MenuItem("Debug Window"))
            {
                debug_window.show = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
void App::open_cartridge()
{
    lutry
    {
        Window::FileDialogFilter filter;
        filter.name = "GameBoy cartridge file";
        const c8* extensions[] = {"gb"};
        filter.extensions = {extensions, 1};
        auto result = Window::open_file_dialog("Select Project File", {&filter, 1});
        if(succeeded(result) && !result.get().empty())
        {
            close_cartridge(); // Close previous cartridge if present.
            Path& path = result.get()[0];
            lulet(f, open_file(path.encode().c_str(), FileOpenFlag::read, FileCreationMode::open_existing));
            lulet(rom_data, load_file_data(f));
            UniquePtr<Emulator> emu(memnew<Emulator>());
            luexp(emu->init(rom_data.data(), rom_data.size()));
            emulator = move(emu);
        }
    }
    lucatch
    {
        Window::message_box("Failed to open cartridge file.", "Load cartridge failed", Window::MessageBoxType::ok, Window::MessageBoxIcon::error);
    }
}
void App::close_cartridge()
{
    emulator.reset();
}