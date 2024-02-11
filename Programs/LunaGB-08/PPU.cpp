#include "PPU.hpp"
#include "Emulator.hpp"

void PPU::increase_ly(Emulator* emu)
{
    ++ly;
    if(ly == lyc)
    {
        set_lyc_flag();
        if(lyc_int_enabled())
        {
            emu->int_flags |= INT_LCD_STAT;
        }
    }
    else
    {
        reset_lyc_flag();
    }
}
void PPU::init()
{
    lcdc = 0x91;
    lcds = 0;
    scroll_y = 0;
    scroll_x = 0;
    ly = 0;
    lyc = 0;
    dma = 0;
    bgp = 0xFC;
    obp0 = 0xFF;
    obp1 = 0xFF;
    wy = 0;
    wx = 0;
    set_mode(PPUMode::oam_scan);

    line_cycles = 0;
    memzero(pixels, sizeof(pixels));
    current_back_buffer = 0;
}
void PPU::tick(Emulator* emu)
{
    if(!enabled()) return;
    ++line_cycles;
    switch(get_mode())
    {
        case PPUMode::oam_scan:
            tick_oam_scan(emu); break;
        case PPUMode::drawing:
            tick_drawing(emu); break;
        case PPUMode::hblank:
            tick_hblank(emu); break;
        case PPUMode::vblank:
            tick_vblank(emu); break;
        default: 
            lupanic(); break;
    }
}
u8 PPU::bus_read(u16 addr)
{
    luassert(addr >= 0xFF40 && addr <= 0xFF4B);
    return ((u8*)(&lcdc))[addr - 0xFF40];
}
void PPU::bus_write(u16 addr, u8 data)
{
    luassert(addr >= 0xFF40 && addr <= 0xFF4B);
    if(addr == 0xFF41) // the lower 3 bits are read only.
    {
        lcds = (lcds & 0x07) | (data & 0xF8);
        return;
    }
    if(addr == 0xFF44) return; // read only.
    ((u8*)(&lcdc))[addr - 0xFF40] = data;
}
void PPU::tick_oam_scan(Emulator* emu)
{
    if(line_cycles >= 80)
    {
        set_mode(PPUMode::drawing);
    }
}
void PPU::tick_drawing(Emulator* emu)
{
    if(line_cycles >= 369)
    {
        set_mode(PPUMode::hblank);
        if(hblank_int_enabled())
        {
            emu->int_flags |= INT_LCD_STAT;
        }
    }
}
void PPU::tick_hblank(Emulator* emu)
{
    if(line_cycles >= PPU_CYCLES_PER_LINE)
    {
        increase_ly(emu);
        if(ly >= PPU_YRES)
        {
            set_mode(PPUMode::vblank);
            emu->int_flags |= INT_VBLANK;
            if(vblank_int_enabled())
            {
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        else
        {
            set_mode(PPUMode::oam_scan);
            if(oam_int_enabled())
            {
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        line_cycles = 0;
    }
}
void PPU::tick_vblank(Emulator* emu)
{
    if(line_cycles >= PPU_CYCLES_PER_LINE)
    {
        increase_ly(emu);
        if(ly >= PPU_LINES_PER_FRAME)
        {
            // move to next frame.
            set_mode(PPUMode::oam_scan);
            ly = 0;
            if(oam_int_enabled())
            {
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        line_cycles = 0;
    }
}