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
}
void CPU::step(Emulator* emu)
{
    if(!halted)
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
    else
    {
        emu->tick(1);
    }
}