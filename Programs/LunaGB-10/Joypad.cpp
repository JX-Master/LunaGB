#include "Joypad.hpp"
#include "Emulator.hpp"

void Joypad::init()
{
    memzero(this);
    p1 = 0xFF;
}
u8 Joypad::get_key_state() const
{
    u8 v = 0xFF;
    if(!bit_test(&p1, 4))
    {
        if(right) bit_reset(&v, 0);
        if(left) bit_reset(&v, 1);
        if(up) bit_reset(&v, 2);
        if(down) bit_reset(&v, 3);
        bit_reset(&v, 4);
    }
    if(!bit_test(&p1, 5))
    {
        if(a) bit_reset(&v, 0);
        if(b) bit_reset(&v, 1);
        if(select) bit_reset(&v, 2);
        if(start) bit_reset(&v, 3);
        bit_reset(&v, 5);
    }
    return v;
}
void Joypad::update(Emulator* emu)
{
    u8 v = get_key_state();
    // Any button is pressed in this update.
    if((bit_test(&p1, 0) && !bit_test(&v, 0)) ||
    (bit_test(&p1, 1) && !bit_test(&v, 1)) ||
    (bit_test(&p1, 2) && !bit_test(&v, 2)) ||
    (bit_test(&p1, 3) && !bit_test(&v, 3)))
    {
        emu->int_flags |= INT_JOYPAD;
    }
    p1 = v;
}
u8 Joypad::bus_read()
{
    return p1;
}
void Joypad::bus_write(u8 v)
{
    // Only update bit 4 and bit 5.
    p1 = (v & 0x30) | (p1 & 0xCF);
    // Refresh key states.
    p1 = get_key_state();
}