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
    dma_active = false;
    dma_offset = 0;
    dma_start_delay = 0;
    line_cycles = 0;
    memzero(pixels, sizeof(pixels));
    current_back_buffer = 0;
}
void PPU::tick(Emulator* emu)
{
    if((emu->clock_cycles % 4) == 0)
    {
        tick_dma(emu);
    }
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
    if(addr == 0xFF46)
    {
        // Enable DMA transfer.
        dma_active = true;
        dma_offset = 0;
        dma_start_delay = 1;
    }
    ((u8*)(&lcdc))[addr - 0xFF40] = data;
}
void PPU::tick_dma(Emulator* emu)
{
    if(!dma_active) return;
    if(dma_start_delay)
    {
        --dma_start_delay;
        return;
    }
    emu->oam[dma_offset] = emu->bus_read((((u16)dma) * 0x100) + dma_offset);
    ++dma_offset;
    dma_active = dma_offset < 0xA0;
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
    // Can be any tick between 0 and 79. 
    // The real PPU finishes OAM scanning in 80 cycles, but we can do it in one cycle.
    if(line_cycles == 1)
    {
        sprites.clear();
        sprites.reserve(10);
        u8 sprite_height = obj_height();
        // Scan all 40 entries.
        for(u8 i = 0; i < 40; ++i)
        {
            if(sprites.size() >= 10)
            {
                // We can hold at most 10 sprites per line.
                break;
            }
            OAMEntry* entry = (OAMEntry*)(emu->oam) + i;
            // Check if this sprite is in this scanline.
            if(entry->y <= ly + 16 && entry->y + sprite_height > ly + 16)
            {
                auto iter = sprites.begin();
                while(iter != sprites.end())
                {
                    if(iter->x > entry->x) break;
                    ++iter;
                }
                sprites.insert(iter, *entry);
            }
        }
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
            obj_queue.clear();
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
void PPU::fetcher_get_sprite_tile(Emulator* emu)
{
    num_fetched_sprites = 0;
    // Load this sprite tile.
    for(u8 i = 0; i < (u8)sprites.size(); ++i)
    {
        i32 sp_x = (i32)sprites[i].x - 8;
        // If the first or last pixel of the sprite row falls in this fetch 
        if(((sp_x >= tile_x_begin) && (sp_x < (tile_x_begin + 8))) ||
            ((sp_x + 7 >= tile_x_begin) && (sp_x + 7 < (tile_x_begin + 8))))
        {
            fetched_sprites[num_fetched_sprites] = sprites[i];
            ++num_fetched_sprites;
        }
        // We can handle at most 3 sprites in one fetch.
        if(num_fetched_sprites >= 3)
        {
            break;
        }
    }
}
void PPU::fetcher_get_sprite_data(Emulator* emu, u8 data_index)
{
    u8 sprite_height = obj_height();
    for(u8 i = 0; i < num_fetched_sprites; ++i)
    {
        u8 ty = (u8)(ly + 16 - fetched_sprites[i].y);
        if(fetched_sprites[i].y_flip())
        {
            // Flip y in tile.
            ty = (sprite_height - 1) - ty;
        }
        u8 tile = fetched_sprites[i].tile;
        if(sprite_height == 16)
        {
            tile &= 0xFE; // Clear the last 1 bit if in double tile mode.
        }
        sprite_fetched_data[(i * 2) + data_index] = emu->bus_read(0x8000 + (tile * 16) + ty * 2 + data_index);
    }
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
void PPU::fetcher_push_sprite_pixels(u8 push_begin, u8 push_end)
{
    for(u32 i = push_begin; i < push_end; ++i)
    {
        ObjectPixel pixel;
        // The default value is one transparent color.
        pixel.color = 0;
        pixel.palette = 0;
        pixel.bg_priority = true;
        if(obj_enable())
        {
            for(u8 s = 0; s < num_fetched_sprites; ++s)
            {
                i32 spx = (i32)fetched_sprites[s].x - 8;
                i32 offset = (i32)(i) - spx;
                if(offset < 0 || offset > 7)
                {
                    // This sprite does not cover this pixel.
                    continue;
                }
                u8 b1 = sprite_fetched_data[s * 2];
                u8 b2 = sprite_fetched_data[s * 2 + 1];
                u8 b = 7 - (u8)offset;
                if(fetched_sprites[s].x_flip())
                {
                   b = (u8)offset;
                }
                u8 lo = (!!(b1 & (1 << b)));
                u8 hi = (!!(b2 & (1 << b))) << 1;
                u8 color = hi | lo;
                if(color == 0)
                {
                    // If this sprite is transparent, we look for the next sprite to blend.
                    continue;
                }
                // Use this pixel.
                pixel.color = color;
                pixel.palette = fetched_sprites[s].dmg_palette() ? obp1 : obp0;
                pixel.bg_priority = fetched_sprites[s].priority();
                break;
            }
        }
        obj_queue.push_back(pixel);
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
    else
    {
        tile_x_begin = fetch_x;
    }
    if(obj_enable())
    {
        fetcher_get_sprite_tile(emu);
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
    if(obj_enable())
    {
        fetcher_get_sprite_data(emu, data_index);
    }
    if(data_index == 0) fetch_state = PPUFetchState::data1;
    else fetch_state = PPUFetchState::idle;
}
void PPU::fetcher_push_pixels()
{
    bool pushed = false;
    if(bgw_queue.size() < 8)
    {
        u8 push_begin = push_x;
        fetcher_push_bgw_pixels();
        u8 push_end = push_x;
        fetcher_push_sprite_pixels(push_begin, push_end);
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
        if (draw_x >= PPU_XRES) return;
        BGWPixel bgw_pixel = bgw_queue.front();
        bgw_queue.pop_front();
        ObjectPixel obj_pixel = obj_queue.front();
        obj_queue.pop_front();
        // Calculate background color.
        u8 bg_color = apply_palette(bgw_pixel.color, bgw_pixel.palette);
        // Draw object if:
        // 1. Color index is not 0 (transparent) and:
        // 2. Background priority is not greater than object priority, or the background color is 00.
        bool draw_obj = obj_pixel.color && (!obj_pixel.bg_priority || bg_color == 0);
        // Calculate obj color.
        u8 obj_color = apply_palette(obj_pixel.color, obj_pixel.palette & 0xFC);
        // Selects the final color.
        u8 color = draw_obj ? obj_color : bg_color;
        // Output pixel.
        switch(color)
        {
            case 0: set_pixel(draw_x, ly, 153, 161, 120, 255); break;
            case 1: set_pixel(draw_x, ly, 87, 93, 67, 255); break;
            case 2: set_pixel(draw_x, ly, 42, 46, 32, 255); break;
            case 3: set_pixel(draw_x, ly, 10, 10, 2, 255); break;
        }
        ++draw_x;
    }
}