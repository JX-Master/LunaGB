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