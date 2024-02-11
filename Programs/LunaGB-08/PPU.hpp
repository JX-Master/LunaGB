#pragma once
#include <Luna/Runtime/MemoryUtils.hpp>

using namespace Luna;

enum class PPUMode : u8
{
    hblank = 0,
    vblank = 1,
    oam_scan = 2,
    drawing = 3
};
constexpr u32 PPU_LINES_PER_FRAME = 154;
constexpr u32 PPU_CYCLES_PER_LINE = 456;
constexpr u32 PPU_YRES = 144;
constexpr u32 PPU_XRES = 160;
struct Emulator;
struct PPU
{
    //! 0xFF40 - LCD control.
    u8 lcdc;
    //! 0xFF41 - LCD status.
    u8 lcds;
    //! 0xFF42 - SCY.
    u8 scroll_y;
    //! 0xFF43 - SCX.
    u8 scroll_x;
    //! 0xFF44 - LY LCD Y coordinate [Read Only].
    u8 ly;
    //! 0xFF45 - LYC LCD Y Compare.
    u8 lyc;
    //! 0xFF46 - DMA value.
    u8 dma;
    //! 0xFF47 - BGP (BG palette data).
    u8 bgp;
    //! 0xFF48 - OBP0 (OBJ0 palette data).
    u8 obp0;
    //! 0xFF49 - OBP1 (OBJ1 palette data).
    u8 obp1;
    //! 0xFF4A - WY (Window Y position plus 7).
    u8 wy;
    //! 0xFF4B - WX (Window X position plus 7).
    u8 wx;

    // PPU internal state.

    //! The number of cycles used for this scan line.
    u32 line_cycles;

    //! Contains the pixel data that should be displayed in the application.
    //! Every pixel of the data is represented by four bytes, arranged in RGBA order.
    //! We use double buffer to prevent tearing when presenting frames.
    u8 pixels[PPU_XRES * PPU_YRES * 4 * 2];
    u8 current_back_buffer;

    bool enabled() const { return bit_test(&lcdc, 7); }

    PPUMode get_mode() const { return (PPUMode)(lcds & 0x03); }
    void set_mode(PPUMode mode)
    {
        lcds &= 0xFC; // clears previous mode.
        lcds |= (u8)(mode);
    }
    void set_lyc_flag() { bit_set(&lcds, 2); }
    void reset_lyc_flag() { bit_reset(&lcds, 2); }
    bool hblank_int_enabled() const { return !!(lcds & (1 << 3)); }
    bool vblank_int_enabled() const { return !!(lcds & (1 << 4)); }
    bool oam_int_enabled() const { return !!(lcds & (1 << 5)); }
    bool lyc_int_enabled() const { return !!(lcds & (1 << 6)); }

    void increase_ly(Emulator* emu);

    void init();
    void tick(Emulator* emu);
    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);

    void tick_oam_scan(Emulator* emu);
    void tick_drawing(Emulator* emu);
    void tick_hblank(Emulator* emu);
    void tick_vblank(Emulator* emu);
};