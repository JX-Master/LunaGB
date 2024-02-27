#include "Emulator.hpp"
#include "Cartridge.hpp"
#include <Luna/Runtime/Log.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Runtime/Time.hpp>

RV Emulator::init(Path cartridge_path, const void* cartridge_data, usize cartridge_data_size)
{
    luassert(cartridge_data && cartridge_data_size);
    this->cartridge_path = cartridge_path;
    rom_data = (byte_t*)memalloc(cartridge_data_size);
    memcpy(rom_data, cartridge_data, cartridge_data_size);
    rom_data_size = cartridge_data_size;
    // Check cartridge data.
    CartridgeHeader* header = get_cartridge_header(rom_data);
    u8 checksum = 0;
    for (u16 address = 0x0134; address <= 0x014C; ++address)
    {
        checksum = checksum - rom_data[address] - 1;
    }
    if(checksum != header->checksum)
    {
        return set_error(BasicError::bad_data(), "The cartridge checksum dismatched. Expected: %u, computed: %u", (u32)header->checksum, (u32)checksum);
    }
    num_rom_banks = (((usize)32) << header->rom_size) / 16;
    // Print cartridge load info.
    c8 title[16];
    snprintf(title, 16, "%s", header->title);
    log_info("LunaGB", "Cartridge Loaded.");
    log_info("LunaGB", "Title    : %s", title);
    log_info("LunaGB", "Type     : %2.2X (%s)", (u32)header->cartridge_type, get_cartridge_type_name(header->cartridge_type));
    log_info("LunaGB", "ROM Size : %u KB", (u32)(32 << header->rom_size));
    log_info("LunaGB", "RAM Size : %2.2X (%s)", (u32)header->ram_size, get_cartridge_ram_size_name(header->ram_size));
    log_info("LunaGB", "LIC Code : %2.2X (%s)", (u32)header->lic_code, get_cartridge_lic_code_name(header->lic_code));
    log_info("LunaGB", "ROM Ver. : %2.2X", (u32)header->version);
    cpu.init();
    memzero(wram, 8_kb);
    memzero(vram, 8_kb);
    memzero(oam, 160);
    memzero(hram, 128);
    int_flags = 0;
    int_enable_flags = 0;
    timer.init();
    serial.init();
    ppu.init();
    joypad.init();
    rtc.init();
    switch(header->ram_size)
    {
        case 2: cram_size = 8_kb; break;
        case 3: cram_size = 32_kb; break;
        case 4: cram_size = 128_kb; break;
        case 5: cram_size = 64_kb; break;
        default: break;
    }
    if(is_cart_mbc2(header->cartridge_type))
    {
        // MBC2 cartridges have fixed 512x4 bits of RAM, which is not shown in header info.
        cram_size = 512;
    }
    if(cram_size)
    {
        cram = (byte_t*)memalloc(cram_size);
        memzero(cram, cram_size);
        if(is_cart_battery(header->cartridge_type))
        {
            load_cartridge_ram_data();
        }
    }
    return ok;
}
void Emulator::update(f64 delta_time)
{
    joypad.update(this);
    if(is_cart_timer(get_cartridge_header(rom_data)->cartridge_type))
    {
        rtc.update(delta_time);
    }
    u64 frame_cycles = (u64)((f32)(4194304.0 * delta_time) * clock_speed_scale);
    u64 end_cycles = clock_cycles + frame_cycles;
    while(clock_cycles < end_cycles)
    {
        if(paused) break;
        cpu.step(this);
    }
}
void Emulator::tick(u32 mcycles)
{
    u32 tick_cycles = mcycles * 4;
    for(u32 i = 0; i < tick_cycles; ++i)
    {
        ++clock_cycles;
        timer.tick(this);
        if((clock_cycles % 512) == 0)
        {
            // Serial is ticked at 8192Hz.
            serial.tick(this);
        }
        ppu.tick(this);
    }
}
void Emulator::close()
{
    if(cram)
    {
        CartridgeHeader* header = get_cartridge_header(rom_data);
        if(is_cart_battery(header->cartridge_type))
        {
            save_cartridge_ram_data();
        }
        memfree(cram);
        cram = nullptr;
        cram_size = 0;
    }
    if (rom_data)
    {
        memfree(rom_data);
        rom_data = nullptr;
        rom_data_size = 0;
        log_info("LunaGB", "Cartridge Unloaded.");
    }
}
u8 Emulator::bus_read(u16 addr)
{
    if(addr <= 0x7FFF)
    {
        // Cartridge ROM.
        return cartridge_read(this, addr);
    }
    if(addr <= 0x9FFF)
    {
        // VRAM.
        return vram[addr - 0x8000];
    }
    if(addr <= 0xBFFF)
    {
        // Cartridge RAM.
        return cartridge_read(this, addr);
    }
    if(addr <= 0xDFFF)
    {
        // Working RAM.
        return wram[addr - 0xC000];
    }
    if(addr >= 0xFE00 && addr <= 0xFE9F)
    {
        return oam[addr - 0xFE00];
    }
    if(addr == 0xFF00)
    {
        return joypad.bus_read();
    }
    if(addr >= 0xFF01 && addr <= 0xFF02)
    {
        return serial.bus_read(addr);
    }
    if(addr >= 0xFF04 && addr <= 0xFF07)
    {
        return timer.bus_read(addr);
    }
    if(addr == 0xFF0F)
    {
        // IF
        return int_flags | 0xE0;
    }
    if(addr >= 0xFF40 && addr <= 0xFF4B)
    {
        return ppu.bus_read(addr);
    }
    if(addr >= 0xFF80 && addr <= 0xFFFE)
    {
        // High RAM.
        return hram[addr - 0xFF80];
    }
    if(addr == 0xFFFF)
    {
        // IE
        return int_enable_flags | 0xE0;
    }
    log_error("LunaGB", "Unsupported bus read address: 0x%04X", (u32)addr);
    return 0xFF;
}
void Emulator::bus_write(u16 addr, u8 data)
{
    if(addr <= 0x7FFF)
    {
        // Cartridge ROM.
        cartridge_write(this, addr, data);
        return;
    }
    if(addr <= 0x9FFF)
    {
        // VRAM.
        vram[addr - 0x8000] = data;
        return;
    }
    if(addr <= 0xBFFF)
    {
        // Cartridge RAM.
        cartridge_write(this, addr, data);
        return;
    }
    if(addr <= 0xDFFF)
    {
        // Working RAM.
        wram[addr - 0xC000] = data;
        return;
    }
    if(addr >= 0xFE00 && addr <= 0xFE9F)
    {
        oam[addr - 0xFE00] = data;
        return;
    }
    if(addr == 0xFF00)
    {
        joypad.bus_write(data);
        return;
    }
    if(addr >= 0xFF01 && addr <= 0xFF02)
    {
        serial.bus_write(addr, data);
        return;
    }
    if(addr >= 0xFF04 && addr <= 0xFF07)
    {
        timer.bus_write(addr, data);
        return;
    }
    if(addr == 0xFF0F)
    {
        // IF
        int_flags = data & 0x1F;
        return;
    }
    if(addr >= 0xFF40 && addr <= 0xFF4B)
    {
        ppu.bus_write(addr, data);
        return;
    }
    if(addr >= 0xFF80 && addr <= 0xFFFE)
    {
        // High RAM.
        hram[addr - 0xFF80] = data;
        return;
    }
    if(addr == 0xFFFF)
    {
        // IE
        int_enable_flags = data & 0x1F;
        return;
    }
    log_error("LunaGB", "Unsupported bus write address: 0x%04X", (u32)addr);
    return;
}
void Emulator::load_cartridge_ram_data()
{
    lutry
    {
        auto path = cartridge_path;
        if (path.empty()) return;
        path.replace_extension("sav");
        lulet(f, open_file(path.encode().c_str(), FileOpenFlag::read, FileCreationMode::open_existing));
        luexp(f->read(cram, cram_size));
        if(is_cart_timer(get_cartridge_header(rom_data)->cartridge_type))
        {
            // Restore RTC.
            luexp(f->read(&rtc, sizeof(RTC)));
            // Read timestamp.
            i64 save_timestamp;
            luexp(f->read(&save_timestamp, sizeof(i64)));
            if(!rtc.halted())
            {
                // Apply delta time between last save time and current time.
                i64 current_timestamp = get_utc_timestamp();
                i64 delta_time = current_timestamp - save_timestamp;
                if(delta_time < 0) delta_time = 0;
                rtc.time += delta_time;
                rtc.update_time_registers();
            }
        }
        log_info("LunaGB", "cartridge RAM data loaded: %s", path.encode().c_str());
    }
    lucatch {}
    return;
}
void Emulator::save_cartridge_ram_data()
{
    lutry
    {
        if (cartridge_path.empty() || !cram) return;
        Path path = cartridge_path;
        path.replace_extension("sav");
        lulet(f, open_file(path.encode().c_str(), FileOpenFlag::write, FileCreationMode::create_always));
        luexp(f->write(cram, cram_size));
        if(is_cart_timer(get_cartridge_header(rom_data)->cartridge_type))
        {
            // Save RTC state.
            luexp(f->write(&rtc, sizeof(RTC)));
            // Save current timestamp.
            i64 timestamp = get_utc_timestamp();
            luexp(f->write(&timestamp, sizeof(i64)));
        }
        log_info("LunaGB", "Save cartridge RAM data to %s.", path.encode().c_str());
    }
    lucatch
    {
        log_error("LunaGB", "Failed to save cartridge RAM data. Game progress may lost!");
    }
}