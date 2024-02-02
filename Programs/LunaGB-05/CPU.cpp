#include "CPU.hpp"
#include "Emulator.hpp"
#include <Luna/Runtime/Log.hpp>
#include "Instructions.hpp"
void CPU::init()
{
    // ref: https://github.com/rockytriton/LLD_gbemu/raw/main/docs/The%20Cycle-Accurate%20Game%20Boy%20Docs.pdf
    af(0x01B0);
    bc(0x0013);
    de(0x00D8);
    hl(0x014D);
    sp = 0xFFFE;
    pc = 0x0100;
    halted = false;
    interrupt_master_enabled = false;
    interrupt_master_enabling_countdown = 0;
}
void CPU::step(Emulator* emu)
{
    if(!halted)
    {
        // Handle interruptions.
        if(interrupt_master_enabled && (emu->int_flags & emu->int_enable_flags))
        {
            service_interrupt(emu);
        }
        else
        {
            // fetch opcode.
            u8 opcode = emu->bus_read(pc);
            // increase counter.
            ++pc;
            // execute opcode.
            instruction_func_t* instruction = instructions_map[opcode];
            if(!instruction)
            {
                log_error("LunaGB", "Instruction 0x%02X not present.", (u32)opcode);
                emu->paused = true;
            }
            else
            {
                instruction(emu);
            }
        }
    }
    else
    {
        emu->tick(1);
        // Wake up CPU if any interruption is pending.
        // This happens even if IME is disabled (interruption_enabled == false).
        if(emu->int_flags & emu->int_enable_flags)
        {
            halted = false;
        }
    }
    // Enable interrupt master when countdown reaches 0.
    if(interrupt_master_enabling_countdown)
    {
        --interrupt_master_enabling_countdown;
        if(!interrupt_master_enabling_countdown)
        {
            interrupt_master_enabled = true;
        }
    }
}
inline void push_16(Emulator* emu, u16 v)
{
    emu->cpu.sp -= 2;
    emu->bus_write(emu->cpu.sp + 1, (u8)((v >> 8) & 0xFF));
    emu->bus_write(emu->cpu.sp, (u8)(v & 0xFF));
}
void CPU::service_interrupt(Emulator* emu)
{
    u8 int_flags = emu->int_flags & emu->int_enable_flags;
    u8 service_int = 0;
    if(int_flags & INT_VBLANK) service_int = INT_VBLANK;
    else if(int_flags & INT_LCD_STAT) service_int = INT_LCD_STAT;
    else if(int_flags & INT_TIMER) service_int = INT_TIMER;
    else if(int_flags & INT_SERIAL) service_int = INT_SERIAL;
    else if(int_flags & INT_JOYPAD) service_int = INT_JOYPAD;
    emu->int_flags &= ~service_int;
    emu->cpu.disable_interrupt_master();
    emu->tick(2);
    push_16(emu, emu->cpu.pc);
    emu->tick(2);
    switch(service_int)
    {
        case INT_VBLANK: emu->cpu.pc = 0x40; break;
        case INT_LCD_STAT: emu->cpu.pc = 0x48; break;
        case INT_TIMER: emu->cpu.pc = 0x50; break;
        case INT_SERIAL: emu->cpu.pc = 0x58; break;
        case INT_JOYPAD: emu->cpu.pc = 0x60; break;
    }
    emu->tick(1);
}