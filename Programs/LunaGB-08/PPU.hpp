#pragma once
#include <Luna/Runtime/RingDeque.hpp>

using namespace Luna;

enum class PPUMode : u8
{
    hblank = 0,
    vblank = 1,
    oam_scan = 2,
    drawing = 3
};
enum class PPUFetchState : u8
{
    tile,
    data0,
    data1,
    idle,
    push
};
struct BGWPixel
{
    //! The color index.
    u8 color;
    //! The palette used for this pixel.
    u8 palette;
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
    //! 0xFF4A - WY (Window Y position).
    u8 wy;
    //! 0xFF4B - WX (Window X position plus 7).
    u8 wx;

    // PPU internal state.

    //! The number of cycles used for this scan line.
    u32 line_cycles;
    //! The FIFO queue for background/window pixels.
    //! Pixels are arranged in RGBA order, R in bit 0...8, A in bit 24...32.
    RingDeque<BGWPixel> bgw_queue;
    //! true when we are fetching window tiles.
    //! false when we are fetching background tiles.
    bool fetch_window;
    //! The window line counter.
    u8 window_line;
    //! The current fetch state.
    PPUFetchState fetch_state;
    //! The X position of the next pixel to fetch in screen coordinates.
    //! Advanced by 8, since we fetch 8 pixels at one time.
    u8 fetch_x;
    //! The fetched background/window tile data offset from PPUFetchState::tile step.
    //! Add this with bgw_data_area() to get the final address.
    u16 bgw_data_addr_offset;
    //! The x position of the first pixel in fetched tile, in screen coordinates.
    //! May be negative if scroll_x is not times of 8.
    i16 tile_x_begin;
    //! The fetched background/window data in PPUFetchState::data0 and PPUFetchState::data1 step.
    u8 bgw_fetched_data[2];
    //! The X position of the next pixel to push to the bgw FIFO.
    u8 push_x;
    //! The X position of the next pixel to draw to the back buffer in screen coordinates.
    //! If draw_x >= PPU_XRES then all pixels are drawn, so we can start HBLANK.
    u8 draw_x;
    //! Contains the pixel data that should be displayed in the application.
    //! Every pixel of the data is represented by four bytes, arranged in RGBA order.
    //! We use double buffer to prevent tearing when presenting frames.
    u8 pixels[PPU_XRES * PPU_YRES * 4 * 2];
    u8 current_back_buffer;
    void set_pixel(i32 x, i32 y, u8 r, u8 g, u8 b, u8 a)
    {
        luassert(x >= 0 || x < PPU_XRES);
        luassert(y >= 0 || y < PPU_YRES);
        u8* dst = pixels + current_back_buffer * PPU_XRES * PPU_YRES * 4;
        u8* pixel = (u8*)pixel_offset(dst, (usize)x, (usize)y, 4, 4 * PPU_XRES);
        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;
        pixel[3] = a;
    }

    bool enabled() const { return bit_test(&lcdc, 7); }
    
    bool bg_window_enable() const { return bit_test(&lcdc, 0); };
    bool window_enable() const { return bit_test(&lcdc, 5); }

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

    u16 bg_map_area() const
    {
        return bit_test(&lcdc, 3) ? 0x9C00 : 0x9800;
    }
    u16 window_map_area() const
    {
        return bit_test(&lcdc, 6) ? 0x9C00 : 0x9800;
    }
    u32 bgw_data_area() const
    {
        return bit_test(&lcdc, 4) ? 0x8000 : 0x8800;
    }
    bool window_visible() const
    {
        return window_enable() && wx <= 166 && wy < PPU_YRES;
    }
    bool is_pixel_window(u8 screen_x, u8 screen_y) const
    {
        return window_visible() && (screen_x + 7 >= wx) && (screen_y >= wy);
    }

    void increase_ly(Emulator* emu);

    void init();
    void tick(Emulator* emu);
    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);

    void tick_oam_scan(Emulator* emu);
    void tick_drawing(Emulator* emu);
    void tick_hblank(Emulator* emu);
    void tick_vblank(Emulator* emu);

    void fetcher_get_tile(Emulator* emu);
    void fetcher_get_data(Emulator* emu, u8 data_index);
    void fetcher_push_pixels();

    void lcd_draw_pixel();
};