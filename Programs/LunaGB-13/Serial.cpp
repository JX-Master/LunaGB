#include "Serial.hpp"
#include "Emulator.hpp"

void Serial::begin_transfer()
{
    transferring = true;
    out_byte = sb;
    transfer_bit = 7;
}
void Serial::process_transfer(Emulator* emu)
{
    sb <<= 1;
    // Set lowest bit to 1.
    ++sb;
    --transfer_bit;
    if(transfer_bit < 0)
    {
        transfer_bit = 0;
        end_transfer(emu);
    }
}
void Serial::end_transfer(Emulator* emu)
{
    output_buffer.push_back(out_byte);
    bit_reset(&sc, 7);
    transferring = false;
    emu->int_flags |= INT_SERIAL;
}
void Serial::tick(Emulator* emu)
{
    if(!transferring && transfer_enable() && is_master())
    {
        begin_transfer();
    }
    else if(transferring)
    {
        process_transfer(emu);
    }
}
u8 Serial::bus_read(u16 addr)
{
    luassert(addr >= 0xFF01 && addr <= 0xFF02);
    if(addr == 0xFF01) return sb;
    if(addr == 0xFF02) return sc;
}
void Serial::bus_write(u16 addr, u8 data)
{
    luassert(addr >= 0xFF01 && addr <= 0xFF02);
    if(addr == 0xFF01)
    {
        sb = data;
        return;
    }
    if(addr == 0xFF02)
    {
        sc = 0x7C | (data & 0x83);
        return;
    }
}