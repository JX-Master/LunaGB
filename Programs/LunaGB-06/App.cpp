#include "App.hpp"
#include <Luna/Runtime/Log.hpp>
#include <Luna/ImGui/ImGui.hpp>
#include <Luna/Window/FileDialog.hpp>
#include <Luna/Window/MessageBox.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Runtime/Time.hpp>

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

        // Clear back buffer.
        lulet(back_buffer, swap_chain->get_current_back_buffer());
        Float4U clear_color = { 0.3f, 0.3f, 0.3f, 1.3f };
        RHI::RenderPassDesc render_pass;
        render_pass.color_attachments[0] = RHI::ColorAttachment(back_buffer, RHI::LoadOp::clear, RHI::StoreOp::store, clear_color);
        cmdbuf->begin_render_pass(render_pass);
        cmdbuf->end_render_pass();
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
App::~App()
{
    // TODO...
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