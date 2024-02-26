#pragma once
#include <Luna/Runtime/Base.hpp>
using namespace Luna;

//! The CartridgeHeader is mapped in range 0x0100 ~ 0x014F in ROM data.
//! See https://gbdev.io/pandocs/The_Cartridge_Header.html
struct CartridgeHeader
{
    u8 entry[4];
    u8 logo[0x30];
    c8 title[16];
    u8 new_lic_code[2];
    u8 sgb_flag;
    u8 cartridge_type;
    u8 rom_size;
    u8 ram_size;
    u8 dest_code;
    u8 lic_code;
    u8 version;
    u8 checksum;
    u8 global_checksum[2];
};

inline CartridgeHeader* get_cartridge_header(byte_t* rom_data)
{
    return (CartridgeHeader*)(rom_data + 0x0100);
}

const c8* get_cartridge_type_name(u8 type);
const c8* get_cartridge_ram_size_name(u8 ram_size_code);
const c8* get_cartridge_lic_code_name(u8 lic_code);

struct Emulator;
u8 cartridge_read(Emulator* emu, u16 addr);
void cartridge_write(Emulator* emu, u16 addr, u8 data);

inline bool is_cart_battery(u8 cartridge_type)
{
    return cartridge_type == 3 || // MBC1+RAM+BATTERY
    cartridge_type == 6 || // MBC2+BATTERY
    cartridge_type == 9 || // ROM+RAM+BATTERY 1
    cartridge_type == 13 || // MMM01+RAM+BATTERY
    cartridge_type == 15 || // MBC3+TIMER+BATTERY
    cartridge_type == 16 || // MBC3+TIMER+RAM+BATTERY 2
    cartridge_type == 19 || // MBC3+RAM+BATTERY 2
    cartridge_type == 27 || // MBC5+RAM+BATTERY
    cartridge_type == 30 || // MBC5+RUMBLE+RAM+BATTERY
    cartridge_type == 34; // MBC7+SENSOR+RUMBLE+RAM+BATTERY
}
inline bool is_cart_mbc1(u8 cartridge_type)
{
    return cartridge_type >= 1 && cartridge_type <= 3;
}
inline bool is_cart_mbc2(u8 cartridge_type)
{
    return cartridge_type >= 5 && cartridge_type <= 6;
}