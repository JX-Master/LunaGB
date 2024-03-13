#include "DebugWindow.hpp"
#include "App.hpp"
#include <Luna/ImGui/ImGui.hpp>
#include <Luna/Window/FileDialog.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Runtime/Log.hpp>
#include <Luna/RHI/Utility.hpp>

void DebugWindow::gui()
{
    if(ImGui::Begin("Debug Window", &show))
    {
        cpu_gui();
        serial_gui();
        tiles_gui();
        ppu_gui();
        apu_gui();
    }
    ImGui::End();
}
const c8* instruction_names[256] = {
    "x00 NOP",
    "x01 LD BC, d16",
    "x02 LD (BC), A",
    "x03 INC BC",
    "x04 INC B",
    "x05 DEC B",
    "x06 LD B, d8",
    "x07 RLCA",
    "x08 LD (a16), SP",
    "x09 ADD HL, BC",
    "x0A LD A, (BC)",
    "x0B DEC BC",
    "x0C INC C",
    "x0D DEC C",
    "x0E LD C, d8",
    "x0F RRCA",
    "x10 STOP 0",
    "x11 LD DE, d16",
    "x12 LD (DE), A",
    "x13 INC DE",
    "x14 INC D",
    "x15 DEC D",
    "x16 LD D, d8",
    "x17 RLA",
    "x18 JR r8",
    "x19 ADD HL, DE",
    "x1A LD A, (DE)",
    "x1B DEC DE",
    "x1C INC E",
    "x1D DEC E",
    "x1E LD E, d8",
    "x1F RRA",
    "x20 JR NZ, r8",
    "x21 LD HL, d16",
    "x22 LD (HL+), A",
    "x23 INC HL",
    "x24 INC H",
    "x25 DEC H",
    "x26 LD H, d8",
    "x27 DAA",
    "x28 JR Z, r8",
    "x29 ADD HL, HL",
    "x2A LD A, (HL+)",
    "x2B DEC HL",
    "x2C INC L",
    "x2D DEC L",
    "x2E LD L, d8",
    "x2F CPL",
    "x30 JR NC, r8",
    "x31 LD SP, d16",
    "x32 LD (HL-), A",
    "x33 INC SP",
    "x34 INC (HL)",
    "x35 DEC (HL)",
    "x36 LD (HL), d8",
    "x37 SCF",
    "x38 JR C, r8",
    "x39 ADD HL, SP",
    "x3A LD A, (HL-)",
    "x3B DEC SP",
    "x3C INC A",
    "x3D DEC A",
    "x3E LD A, d8",
    "x3F CCF",
    "x40 LD B, B",
    "x41 LD B, C",
    "x42 LD B, D",
    "x43 LD B, E",
    "x44 LD B, H",
    "x45 LD B, L",
    "x46 LD B, (HL)",
    "x47 LD B, A",
    "x48 LD C, B",
    "x49 LD C, C",
    "x4A LD C, D",
    "x4B LD C, E",
    "x4C LD C, H",
    "x4D LD C, L",
    "x4E LD C, (HL)",
    "x4F LD C, A",
    "x50 LD D, B",
    "x51 LD D, C",
    "x52 LD D, D",
    "x53 LD D, E",
    "x54 LD D, H",
    "x55 LD D, L",
    "x56 LD D, (HL)",
    "x57 LD D, A",
    "x58 LD E, B",
    "x59 LD E, C",
    "x5A LD E, D",
    "x5B LD E, E",
    "x5C LD E, H",
    "x5D LD E, L",
    "x5E LD E, (HL)",
    "x5F LD E, A",
    "x60 LD H, B",
    "x61 LD H, C",
    "x62 LD H, D",
    "x63 LD H, E",
    "x64 LD H, H",
    "x65 LD H, L",
    "x66 LD H, (HL)",
    "x67 LD H, A",
    "x68 LD L, B",
    "x69 LD L, C",
    "x6A LD L, D",
    "x6B LD L, E",
    "x6C LD L, H",
    "x6D LD L, L",
    "x6E LD L, (HL)",
    "x6F LD L, A",
    "x70 LD (HL), B",
    "x71 LD (HL), C",
    "x72 LD (HL), D",
    "x73 LD (HL), E",
    "x74 LD (HL), H",
    "x75 LD (HL), L",
    "x76 HALT",
    "x77 LD (HL), A",
    "x78 LD A, B",
    "x79 LD A, C",
    "x7A LD A, D",
    "x7B LD A, E",
    "x7C LD A, H",
    "x7D LD A, L",
    "x7E LD A, (HL)",
    "x7F LD A, A",
    "x80 ADD A, B",
    "x81 ADD A, C",
    "x82 ADD A, D",
    "x83 ADD A, E",
    "x84 ADD A, H",
    "x85 ADD A, L",
    "x86 ADD A, (HL)",
    "x87 ADD A, A",
    "x88 ADC A, B",
    "x89 ADC A, C",
    "x8A ADC A, D",
    "x8B ADC A, E",
    "x8C ADC A, H",
    "x8D ADC A, L",
    "x8E ADC A, (HL)",
    "x8F ADC A, A",
    "x90 SUB B",
    "x91 SUB C",
    "x92 SUB D",
    "x93 SUB E",
    "x94 SUB H",
    "x95 SUB L",
    "x96 SUB (HL)",
    "x97 SUB A",
    "x98 SBC A, B",
    "x99 SBC A, C",
    "x9A SBC A, D",
    "x9B SBC A, E",
    "x9C SBC A, H",
    "x9D SBC A, L",
    "x9E SBC A, (HL)",
    "x9F SBC A, A",
    "xA0 AND B",
    "xA1 AND C",
    "xA2 AND D",
    "xA3 AND E",
    "xA4 AND H",
    "xA5 AND L",
    "xA6 AND (HL)",
    "xA7 AND A",
    "xA8 XOR B",
    "xA9 XOR C",
    "xAA XOR D",
    "xAB XOR E",
    "xAC XOR H",
    "xAD XOR L",
    "xAE XOR (HL)",
    "xAF XOR A",
    "xB0 OR B",
    "xB1 OR C",
    "xB2 OR D",
    "xB3 OR E",
    "xB4 OR H",
    "xB5 OR L",
    "xB6 OR (HL)",
    "xB7 OR A",
    "xB8 CP B",
    "xB9 CP C",
    "xBA CP D",
    "xBB CP E",
    "xBC CP H",
    "xBD CP L",
    "xBE CP (HL)",
    "xBF CP A",
    "xC0 RET NZ",
    "xC1 POP BC",
    "xC2 JP NZ, a16",
    "xC3 JP a16",
    "xC4 CALL NZ, a16",
    "xC5 PUSH BC",
    "xC6 ADD A, d8",
    "xC7 RST 00H",
    "xC8 RET Z",
    "xC9 RET",
    "xCA JP Z, a16",
    "xCB PREFIX CB",
    "xCC CALL Z, a16",
    "xCD CALL a16",
    "xCE ADC A, d8",
    "xCF RST 08H",
    "xD0 RET NC",
    "xD1 POP DE",
    "xD2 JP NC, a16",
    "xD3 NULL",
    "xD4 CALL NC, a16",
    "xD5 PUSH DE",
    "xD6 SUB d8",
    "xD7 RST 10H",
    "xD8 RET C",
    "xD9 RETI",
    "xDA JP C, a16",
    "xDB NULL",
    "xDC CALL C, a16",
    "xDD NULL",
    "xDE SBC A, d8",
    "xDF RST 18H",
    "xE0 LDH (a8), A",
    "xE1 POP HL",
    "xE2 LD (C), A",
    "xE3 NULL",
    "xE4 NULL",
    "xE5 PUSH HL",
    "xE6 AND d8",
    "xE7 RST 20H",
    "xE8 ADD SP, r8",
    "xE9 JP (HL)",
    "xEA LD (a16), A",
    "xEB NULL",
    "xEC NULL",
    "xED NULL",
    "xEE XOR d8",
    "xEF RST 28H",
    "xF0 LDH A, (a8)",
    "xF1 POP AF",
    "xF2 LD A, (C)",
    "xF3 DI",
    "xF4 NULL",
    "xF5 PUSH AF",
    "xF6 OR d8",
    "xF7 RST 30H",
    "xF8 LD HL, SP+r8",
    "xF9 LD SP, HL",
    "xFA LD A, (a16)",
    "xFB EI",
    "xFC NULL",
    "xFD NULL",
    "xFE CP d8",
    "xFF RST 38H"
};
const c8* get_opcode_name(u8 opcode)
{
    return instruction_names[opcode];
}
void DebugWindow::cpu_gui()
{
    if(g_app->emulator)
    {
        if(ImGui::CollapsingHeader("CPU Info"))
        {
            auto& cpu = g_app->emulator->cpu;
            if(ImGui::BeginTable("Byte registers", 6, ImGuiTableFlags_Borders))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AF");
                ImGui::TableNextColumn();
                ImGui::Text("%4.4X", (u32)cpu.af());
                ImGui::TableNextColumn();
                ImGui::Text("A");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.a);
                ImGui::TableNextColumn();
                ImGui::Text("F");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("BC");
                ImGui::TableNextColumn();
                ImGui::Text("%4.4X", (u32)cpu.bc());
                ImGui::TableNextColumn();
                ImGui::Text("B");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.b);
                ImGui::TableNextColumn();
                ImGui::Text("C");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.c);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("DE");
                ImGui::TableNextColumn();
                ImGui::Text("%4.4X", (u32)cpu.de());
                ImGui::TableNextColumn();
                ImGui::Text("D");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.d);
                ImGui::TableNextColumn();
                ImGui::Text("E");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.e);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("HL");
                ImGui::TableNextColumn();
                ImGui::Text("%4.4X", (u32)cpu.hl());
                ImGui::TableNextColumn();
                ImGui::Text("H");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.h);
                ImGui::TableNextColumn();
                ImGui::Text("L");
                ImGui::TableNextColumn();
                ImGui::Text("%2.2X", (u32)cpu.l);

                ImGui::EndTable();
            }
            if(ImGui::BeginTable("Word registers", 4, ImGuiTableFlags_Borders))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("PC");
                ImGui::TableNextColumn();
                ImGui::Text("%4.4X", (u32)cpu.pc);
                ImGui::TableNextColumn();
                ImGui::Text("SP");
                ImGui::TableNextColumn();
                ImGui::Text("%4.4X", (u32)cpu.sp);

                ImGui::EndTable();
            }
            ImGui::Text("Flags");
            if(ImGui::BeginTable("Flags", 4, ImGuiTableFlags_Borders))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Z");
                ImGui::TableNextColumn();
                ImGui::Text("N");
                ImGui::TableNextColumn();
                ImGui::Text("H");
                ImGui::TableNextColumn();
                ImGui::Text("C");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(cpu.fz() ? "1" : "0");
                ImGui::TableNextColumn();
                ImGui::Text(cpu.fn() ? "1" : "0");
                ImGui::TableNextColumn();
                ImGui::Text(cpu.fh() ? "1" : "0");
                ImGui::TableNextColumn();
                ImGui::Text(cpu.fc() ? "1" : "0");

                ImGui::EndTable();
            }
            ImGui::Text("* All register values are represented in hexadecimal.");
            if(cpu.halted)
            {
                ImGui::Text("CPU Halted.");
            }
            ImGui::DragFloat("CPU Speed Scale", &g_app->emulator->clock_speed_scale, 0.001f);
        }
        if(ImGui::CollapsingHeader("CPU Stepping"))
        {
            bool cpu_stepping_enabled = g_app->emulator->paused;
            if(!cpu_stepping_enabled)
            {
                ImGui::Text("CPU stepping is enabled only when the game is paused.");
            }
            else
            {
                ImGui::Text("Next instruction: %s", get_opcode_name(g_app->emulator->bus_read(g_app->emulator->cpu.pc)));
                if(ImGui::Button("Step CPU"))
                {
                    g_app->emulator->cpu.step(g_app->emulator.get());
                }
            }
        }
        if(ImGui::CollapsingHeader("CPU Logging"))
        {
            if(ImGui::Button("Clear"))
            {
                cpu_log.clear();
            }
            if(cpu_logging)
            {
                if(ImGui::Button("Stop logging"))
                {
                    cpu_logging = false;
                }
            }
            else
            {
                if(ImGui::Button("Start logging"))
                {
                    cpu_logging = true;
                }
            }
            if(ImGui::Button("Save"))
            {
                Window::FileDialogFilter filter;
                filter.name = "Text file";
                const c8* extension = "txt";
                filter.extensions = {&extension, 1};
                auto rpath = Window::save_file_dialog("Save", {&filter, 1});
                if (succeeded(rpath) && !rpath.get().empty())
                {
                    if(rpath.get().extension() == Name())
                    {
                        rpath.get().replace_extension("txt");
                    }
                    auto f = open_file(rpath.get().encode().c_str(), FileOpenFlag::write, FileCreationMode::create_always);
                    if(succeeded(f))
                    {
                        auto _ = f.get()->write(cpu_log.c_str(), cpu_log.size(), nullptr);
                    }
                }
            }
            ImGui::Text("Log size: %llu bytes.", (u64)cpu_log.size());
        }
    }
}
void DebugWindow::serial_gui()
{
    if(g_app->emulator)
    {
        // Read serial data.
        while(!g_app->emulator->serial.output_buffer.empty())
        {
            u8 data = g_app->emulator->serial.output_buffer.front();
            g_app->emulator->serial.output_buffer.pop_front();
            serial_data.push_back(data);
        }
    }
    if(ImGui::CollapsingHeader("Serial data"))
    {
        ImGui::Text("Serial Data:");
        if(ImGui::BeginChild("Serial Data", ImVec2(500.0f, 100.0f), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX))
        {
            ImGui::TextUnformatted((c8*)serial_data.begin(), (c8*)serial_data.end());
        }
        ImGui::EndChild();
        if(ImGui::Button("Clear"))
        {
            serial_data.clear();
        }
    }
}
inline void decode_tile_line(const u8 data[2], u8 dst_color[32])
{
    for(i32 b = 7; b >= 0; --b)
    {
        u8 lo = (!!(data[0] & (1 << b)));
        u8 hi = (!!(data[1] & (1 << b))) << 1;
        u8 color = hi | lo;
        // convert color.
        switch(color)
        {
            case 0: color = 0xFF; break;
            case 1: color = 0xAA; break;
            case 2: color = 0x55; break;
            case 3: color = 0x00; break;
            default: lupanic(); break;
        }
        dst_color[(7 - b) * 4] = color;
        dst_color[(7 - b) * 4 + 1] = color;
        dst_color[(7 - b) * 4 + 2] = color;
        dst_color[(7 - b) * 4 + 3] = 0xFF;
    }
}
void DebugWindow::tiles_gui()
{
    if(g_app->emulator)
    {
        if(ImGui::CollapsingHeader("Tiles"))
        {
            u32 width = 16 * 8;
            u32 height = 24 * 8;
            // Create texture if not present.
            if(!tile_texture)
            {
                auto tex = g_app->rhi_device->new_texture(RHI::MemoryType::local, 
                        RHI::TextureDesc::tex2d(RHI::Format::rgba8_unorm, RHI::TextureUsageFlag::copy_dest | RHI::TextureUsageFlag::read_texture,
                            width, height, 1, 1));
                if(failed(tex))
                {
                    log_error("LunaGB", "Failed to create texture for tile inspector : %s", explain(tex.errcode()));
                    return;
                }
                tile_texture = tex.get();
            }
            // Update texture data.
            usize num_pixel_bytes = width * height * 4;
            usize row_pitch = width * 4;
            Blob pixel_bytes(num_pixel_bytes);
            u8* pixels = pixel_bytes.data();
            for(u32 y = 0; y < height / 8; ++y)
            {
                for(u32 x = 0; x < width / 8; ++x)
                {
                    u32 tile_index = y * width / 8 + x;
                    usize tile_color_begin = y * row_pitch * 8 + x * 8 * 4;
                    for(u32 line = 0; line < 8; ++line)
                    {
                        decode_tile_line(g_app->emulator->vram + tile_index * 16 + line * 2, pixels + tile_color_begin + line * row_pitch);
                    }
                }
            }
            auto r = RHI::copy_resource_data(g_app->cmdbuf, {
                RHI::CopyResourceData::write_texture(tile_texture, {0, 0}, 0, 0, 0, pixels, row_pitch, num_pixel_bytes, width, height, 1)
            });
            if(failed(r))
            {
                log_error("LunaGB", "Failed to upload texture data for tile inspector : %s", explain(r.errcode()));
                ImGui::End();
                return;
            }
            // Draw.
            ImGui::Image(tile_texture, {(f32)(width * 4), (f32)(height * 4)});
        }
    }
}
void DebugWindow::ppu_gui()
{
    if (g_app->emulator)
    {
        if (ImGui::CollapsingHeader("PPU"))
        {
            ImGui::Text("PPU: %s", g_app->emulator->ppu.enabled() ? "Enabled" : "Disabled");
            ImGui::Text("BG & Window: %s", g_app->emulator->ppu.bg_window_enable() ? "Enabled" : "Disabled");
            //ImGui::Text("Window: %s", g_app->emulator->ppu.window_enable() ? "Enabled" : "Disabled");
            //ImGui::Text("OBJ: %s", g_app->emulator->ppu.obj_enable() ? "Enabled" : "Disabled");
            ImGui::Text("ScrollX: %u, ScrollY: %u", (u32)g_app->emulator->ppu.scroll_x, (u32)g_app->emulator->ppu.scroll_y);
            ImGui::Text("WindowX + 7: %u, WindowY: %u", (u32)g_app->emulator->ppu.wx, (u32)g_app->emulator->ppu.wy);
            ImGui::Text("HBlank Int: %s", g_app->emulator->ppu.hblank_int_enabled() ? "Enabled" : "Disabled");
            ImGui::Text("VBlank Int: %s", g_app->emulator->ppu.vblank_int_enabled() ? "Enabled" : "Disabled");
            ImGui::Text("OAM Int: %s", g_app->emulator->ppu.oam_int_enabled() ? "Enabled" : "Disabled");
            ImGui::Text("LYC Int: %s", g_app->emulator->ppu.lyc_int_enabled() ? "Enabled" : "Disabled");
            ImGui::Text("PPU states change very fast, please slow down clock speed to debug.");
            PPUMode mode = g_app->emulator->ppu.get_mode();
            switch (mode)
            {
            case PPUMode::hblank:
                ImGui::Text("Mode: HBLANK (0)");
                break;
            case PPUMode::vblank:
                ImGui::Text("Mode: VBLANK (1)");
                break;
            case PPUMode::oam_scan:
                ImGui::Text("Mode: OAM_SCAN (2)");
                break;
            case PPUMode::drawing:
                ImGui::Text("Mode: DRAWING (3)");
                break;
            }
            ImGui::Text("LY: %u", (u32)g_app->emulator->ppu.ly);
            ImGui::Text("LY Compare: %u", (u32)g_app->emulator->ppu.lyc);
        }
    }
}
void DebugWindow::apu_gui()
{
    if (g_app->emulator)
    {
        if (ImGui::CollapsingHeader("APU"))
        {
            if(ImGui::CollapsingHeader("Master Control"))
            {
                ImGui::Text("Audio %s", g_app->emulator->apu.is_enabled() ? "Enabled" : "Disabled");
                ImGui::Text("Channel 1 %s", g_app->emulator->apu.ch1_enabled() ? "Enabled" : "Disabled");
                ImGui::Text("Channel 2 %s", g_app->emulator->apu.ch2_enabled() ? "Enabled" : "Disabled");
                ImGui::Text("Left channel");
                ImGui::Text("Volume: %u/7", (u32)g_app->emulator->apu.left_volume());
                ImGui::Text("Channel 1 %s", g_app->emulator->apu.ch1_l_enabled() ? "Enabled" : "Disabled");
                ImGui::Text("Channel 2 %s", g_app->emulator->apu.ch2_l_enabled() ? "Enabled" : "Disabled");
                ImGui::Text("Right channel");
                ImGui::Text("Volume: %u/7", (u32)g_app->emulator->apu.right_volume());
                ImGui::Text("Channel 1 %s", g_app->emulator->apu.ch1_r_enabled() ? "Enabled" : "Disabled");
                ImGui::Text("Channel 2 %s", g_app->emulator->apu.ch2_r_enabled() ? "Enabled" : "Disabled");
            }
            if(ImGui::CollapsingHeader("Audio Channel 1"))
            {
                ImGui::Text("Wave Duty: %u", (u32)g_app->emulator->apu.ch1_wave_type());
                ImGui::Text("Period: %u", (u32)g_app->emulator->apu.ch1_period());
                ImGui::Text("Frequency : %f", 131072.0f / (2048.0f - (f32)g_app->emulator->apu.ch1_period()));
                ImGui::Text("Current Volume: %u/16", (u32)g_app->emulator->apu.ch1_volume);
                ImGui::Text("Initial Volume: %u/16", (u32)g_app->emulator->apu.ch1_initial_volume());
                if(g_app->emulator->apu.ch1_envelope_iteration_pace != 0)
                {
                    ImGui::Text("Envelope Direction: %s", g_app->emulator->apu.ch1_envelope_iteration_increase ? "Increase" : "Decrease");
                    ImGui::Text("Envelope Sweep Pace: %u", (u32)g_app->emulator->apu.ch1_envelope_iteration_pace);
                }
                else
                {
                    ImGui::Text("Envelope Disabled");
                    ImGui::Text(" ");
                }
                ImGui::Text("Sweep Pace: %u", (u32)g_app->emulator->apu.ch1_sweep_pace());
                ImGui::Text("Sweep Step: %u", (u32)g_app->emulator->apu.ch1_sweep_individual_step());
            }
            if(ImGui::CollapsingHeader("Audio Channel 2"))
            {
                ImGui::Text("Wave Duty: %u", (u32)g_app->emulator->apu.ch2_wave_type());
                ImGui::Text("Period: %u", (u32)g_app->emulator->apu.ch2_period());
                ImGui::Text("Frequency : %f", 131072.0f / (2048.0f - (f32)g_app->emulator->apu.ch2_period()));
                ImGui::Text("Current Volume: %u/16", (u32)g_app->emulator->apu.ch2_volume);
                ImGui::Text("Initial Volume: %u/16", (u32)g_app->emulator->apu.ch2_initial_volume());
                if(g_app->emulator->apu.ch2_envelope_iteration_pace)
                {
                    ImGui::Text("Envelope Direction: %s", g_app->emulator->apu.ch2_envelope_iteration_increase ? "Increase" : "Decrease");
                    ImGui::Text("Envelope Sweep Pace: %u", (u32)g_app->emulator->apu.ch2_envelope_iteration_pace);
                }
                else
                {
                    ImGui::Text("Envelope Disabled");
                    ImGui::Text(" ");
                }
            }
            if(ImGui::CollapsingHeader("Audio Channel 3"))
            {
                f32 waveform[32];
                for(u8 i = 0; i < 32; ++i)
                {
                    waveform[i] = (f32)g_app->emulator->apu.ch3_wave_pattern(i);
                }
                ImGui::PlotLines("Waveform", waveform, 32, 0, NULL, 0.0f, 15.0f);
                ImGui::Text("Period: %u", (u32)g_app->emulator->apu.ch3_period());
                u8 output_level = g_app->emulator->apu.ch3_output_level();
                switch(output_level)
                {
                    case 0: ImGui::Text("Volume: 0%%"); break;
                    case 1: ImGui::Text("Volume: 100%%"); break;
                    case 2: ImGui::Text("Volume: 50%%"); break;
                    case 3: ImGui::Text("Volume: 25%%"); break;
                    default: break;
                }
            }
        }
    }
}