#include "PPU.hpp"
#include "Emulator.hpp"

void PPU::increase_ly(Emulator* emu)
{
    if(window_visible() && ly >= wy && 
       (u16)ly < (u16)(wy + PPU_YRES))
    {
        ++window_line;
    }
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
    window_line = 0;
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
    if(addr == 0xFF40 && enabled() && !bit_test(&data, 7))
    {
        // Reset mode to HBLANK.
        lcds &= 0x7C;
        // Reset LY.
        ly = 0;
        window_line = 0;
        line_cycles = 0;
    }
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
        fetch_window = false;
        fetch_state = PPUFetchState::tile;
        fetch_x = 0;
        push_x = 0;
        draw_x = 0;
    }
}
void PPU::tick_drawing(Emulator* emu)
{
    // The fetcher is ticked once per 2 cycles.
    if((line_cycles % 2) == 0)
    {
        switch(fetch_state)
        {
            case PPUFetchState::tile:
                fetcher_get_tile(emu); break;
            case PPUFetchState::data0:
                fetcher_get_data(emu, 0); break;
            case PPUFetchState::data1:
                fetcher_get_data(emu, 1); break;
            case PPUFetchState::idle:
                fetch_state = PPUFetchState::push; break;
            case PPUFetchState::push:
                fetcher_push_pixels(); break;
            default: lupanic(); break;
        }
        if(draw_x >= PPU_XRES)
        {
            luassert(line_cycles >= 252 && line_cycles <= 369);
            set_mode(PPUMode::hblank);
            if(hblank_int_enabled())
            {
                emu->int_flags |= INT_LCD_STAT;
            }
            bgw_queue.clear();
        }
    }
    // LCD driver is ticked once per cycle.
    lcd_draw_pixel();
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
            current_back_buffer = (current_back_buffer + 1) % 2;
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
            window_line = 0;
            if(oam_int_enabled())
            {
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        line_cycles = 0;
    }
}
void PPU::fetcher_get_background_tile(Emulator* emu)
{
    // The y position of the next pixel to fetch relative to 256x256 tile map origin.
    u8 map_y = ly + scroll_y;
    // The x position of the next pixel to fetch relative to 256x256 tile map origin.
    u8 map_x = fetch_x + scroll_x;
    // The address to read map index.
    // ((map_y / 8) * 32) : 32 bytes per row in tile maps.
    u16 addr = bg_map_area() + (map_x / 8) + ((map_y / 8) * 32);
    // Read tile index.
    u8 tile_index = emu->bus_read(addr);
    if(bgw_data_area() == 0x8800)
    {
        // If LCDC.4=0, then range 0x9000~0x97FF is mapped to [0, 127], and range 0x8800~0x8FFF is mapped to [128, 255].
        // We can achieve this by simply add 128 to the fetched data, which will overflow and reset the value if greater 
        // than 127.
        tile_index += 128;
    }
    // Calculate data address offset from bgw data area beginning.
    // tile_index * 16 : every tile takes 16 bytes.
    // (map_y % 8) * 2 : every row takes 2 bytes.
    bgw_data_addr_offset = ((u16)tile_index * 16) + (u16)(map_y % 8) * 2;
    // Calculate tile X position.
    i32 tile_x = (i32)(fetch_x) + (i32)(scroll_x);
    tile_x = (tile_x / 8) * 8 - (i32)scroll_x;
    tile_x_begin = (i16)tile_x;
}
void PPU::fetcher_get_window_tile(Emulator* emu)
{
    u8 window_x = (fetch_x + 7 - wx);
    u8 window_y = window_line;
    u16 window_addr = window_map_area() + (window_x / 8) + ((window_y / 8) * 32);
    u8 tile_index = emu->bus_read(window_addr);
    if(bgw_data_area() == 0x8800)
    {
        // If LCDC.4=0, then range 0x9000~0x97FF is mapped to [0, 127], and range 0x8800~0x8FFF is mapped to [128, 255].
        // We can achieve this by simply add 128 to the fetched data, which will overflow and reset the value if greater 
        // than 127.
        tile_index += 128;
    }
    // Calculate data address offset from bgw data area beginning.
    // tile_index * 16 : every tile takes 16 bytes.
    // (window_tile_y % 8) * 2 : every row takes 2 bytes.
    bgw_data_addr_offset = ((u16)tile_index * 16) + (u16)(window_y % 8) * 2;
    // Calculate tile X position.
    i32 tile_x = (i32)(fetch_x) - ((i32)(wx) - 7);
    tile_x = (tile_x / 8) * 8 + (i32)(wx) - 7;
    tile_x_begin = (i16)tile_x;
}
void PPU::fetcher_push_bgw_pixels()
{
    // Load tile data.
    u8 b1 = bgw_fetched_data[0];
    u8 b2 = bgw_fetched_data[1];
    // Process every pixel in this tile.
    for(u32 i = 0; i < 8; ++i)
    {
        // Skip pixels not in the screen.
        // Pushing pixels when tile_x_begin + i >= PPU_XRES is ok and 
        // they will not be drawn by the LCD driver, and all undrawn pixels
        // will be discarded when HBLANK mode is entered.
        if(tile_x_begin + (i32)i < 0)
        {
            continue;
        }
        // If this is a window pixel, we reset fetcher to fetch window and discard remaining pixels.
        if(!fetch_window && is_pixel_window(push_x, ly))
        {
            fetch_window = true;
            fetch_x = push_x;
            break;
        }
        // Now we can stream pixel.
        BGWPixel pixel;
        if(bg_window_enable())
        {
            u8 b = 7 - i;
            u8 lo = (!!(b1 & (1 << b)));
            u8 hi = (!!(b2 & (1 << b))) << 1;
            pixel.color = hi | lo;
            pixel.palette = bgp;
        }
        else
        {
            pixel.color = 0;
            pixel.palette = 0;
        }
        bgw_queue.push_back(pixel);
        ++push_x;
    }
}
void PPU::fetcher_get_tile(Emulator* emu)
{
    if(bg_window_enable())
    {
        if(fetch_window)
        {
            fetcher_get_window_tile(emu);
        }
        else
        {
            fetcher_get_background_tile(emu);
        }
    }
    fetch_state = PPUFetchState::data0;
    fetch_x += 8;
}
void PPU::fetcher_get_data(Emulator* emu, u8 data_index)
{
    if(bg_window_enable())
    {
        bgw_fetched_data[data_index] = emu->bus_read(bgw_data_area() + bgw_data_addr_offset + data_index);
    }
    if(data_index == 0) fetch_state = PPUFetchState::data1;
    else fetch_state = PPUFetchState::idle;
}
void PPU::fetcher_push_pixels()
{
    bool pushed = false;
    if(bgw_queue.size() < 8)
    {
        fetcher_push_bgw_pixels();
        pushed = true;
    }
    if(pushed)
    {
        fetch_state = PPUFetchState::tile;
    }
}
inline u8 apply_palette(u8 color, u8 palette)
{
    switch(color)
    {
        case 0: color = palette & 0x03; break;
        case 1: color = ((palette >> 2) & 0x03); break;
        case 2: color = ((palette >> 4) & 0x03); break;
        case 3: color = ((palette >> 6) & 0x03); break;
        default: lupanic(); break;
    }
    return color;
}
void PPU::lcd_draw_pixel()
{
    // The LCD driver is drived by BGW queue only, it works when at least 8 pixels are in BGW queue.
    if(bgw_queue.size() >= 8) 
    {
        BGWPixel bgw_pixel = bgw_queue.front();
        bgw_queue.pop_front();
        // Calculate background color.
        u8 bg_color = apply_palette(bgw_pixel.color, bgw_pixel.palette);
        // Output pixel.
        switch(bg_color)
        {
            case 0: set_pixel(draw_x, ly, 153, 161, 120, 255); break;
            case 1: set_pixel(draw_x, ly, 87, 93, 67, 255); break;
            case 2: set_pixel(draw_x, ly, 42, 46, 32, 255); break;
            case 3: set_pixel(draw_x, ly, 10, 10, 2, 255); break;
        }
        ++draw_x;
    }
}