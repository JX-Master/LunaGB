#include "DebugWindow.hpp"
#include "App.hpp"
#include <Luna/ImGui/ImGui.hpp>
#include <Luna/Window/FileDialog.hpp>
#include <Luna/Runtime/File.hpp>

void DebugWindow::gui()
{
    if(ImGui::Begin("Debug Window", &show))
    {
        cpu_gui();
        serial_gui();
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
        if(ImGui::CollapsingHeader("CPU Registers"))
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
        ImGui::DragFloat("CPU Speed Scale", &g_app->emulator->clock_speed_scale, 0.001f);
    }
}
void DebugWindow::serial_gui()
{
    if(g_app->emulator)
    {
        // Read serial data.
        if(!g_app->emulator->serial.output_buffer.empty())
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