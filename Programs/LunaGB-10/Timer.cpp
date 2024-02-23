#include "Timer.hpp"
#include "Emulator.hpp"

void Timer::tick(Emulator* emu)
{
    // Increase DIV.
    u16 prev_div = div;
    ++div;
    if(tima_enabled())
    {
        // Check whether we need to increase TIMA in this tick.
        bool tima_update = false;
        switch(clock_select())
        {
            case 0:
                tima_update = (prev_div & (1 << 9)) && (!(div & (1 << 9)));
                break;
            case 1:
                tima_update = (prev_div & (1 << 3)) && (!(div & (1 << 3)));
                break;
            case 2:
                tima_update = (prev_div & (1 << 5)) && (!(div & (1 << 5)));
                break;
            case 3:
                tima_update = (prev_div & (1 << 7)) && (!(div & (1 << 7)));
                break;
        }
        if (tima_update) 
        {
            if (tima == 0xFF) 
            {
                emu->int_flags |= INT_TIMER;
                tima = tma;
            }
            else
            {
                tima++;
            }
        }
    }
}
u8 Timer::bus_read(u16 addr)
{
    luassert(addr >= 0xFF04 && addr <= 0xFF07);
    if(addr == 0xFF04) return read_div();
    if(addr == 0xFF05) return tima;
    if(addr == 0xFF06) return tma;
    if(addr == 0xFF07) return tac;
}
void Timer::bus_write(u16 addr, u8 data)
{
    luassert(addr >= 0xFF04 && addr <= 0xFF07);
    if(addr == 0xFF04)
    {
        div = 0;
        return;
    }
    if(addr == 0xFF05)
    {
        tima = data;
        return;
    }
    if(addr == 0xFF06)
    {
        tma = data;
        return;
    }
    if(addr == 0xFF07)
    {
        tac = 0xF8 | (data & 0x07);
        return;
    }
}