#include "Emulator.hpp"
#include "Cartridge.hpp"
#include <Luna/Runtime/Log.hpp>

RV Emulator::init(const void* cartridge_data, usize cartridge_data_size)
{
    luassert(cartridge_data && cartridge_data_size);
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
    int_flags = 0;
    int_enable_flags = 0;
    return ok;
}
void Emulator::update(f64 delta_time)
{
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
    }
}
void Emulator::close()
{
    if(rom_data)
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
    if(addr == 0xFF0F)
    {
        // IF
        return int_flags | 0xE0;
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
    if(addr == 0xFF0F)
    {
        // IF
        int_flags = data & 0x1F;
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