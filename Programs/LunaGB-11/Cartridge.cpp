#include "Cartridge.hpp"
#include "Emulator.hpp"
#include <Luna/Runtime/Log.hpp>
static const c8* ROM_TYPES[] = {
    "ROM ONLY",
    "MBC1",
    "MBC1+RAM",
    "MBC1+RAM+BATTERY",
    "0x04 ???",
    "MBC2",
    "MBC2+BATTERY",
    "0x07 ???",
    "ROM+RAM 1",
    "ROM+RAM+BATTERY 1",
    "0x0A ???",
    "MMM01",
    "MMM01+RAM",
    "MMM01+RAM+BATTERY",
    "0x0E ???",
    "MBC3+TIMER+BATTERY",
    "MBC3+TIMER+RAM+BATTERY 2",
    "MBC3",
    "MBC3+RAM 2",
    "MBC3+RAM+BATTERY 2",
    "0x14 ???",
    "0x15 ???",
    "0x16 ???",
    "0x17 ???",
    "0x18 ???",
    "MBC5",
    "MBC5+RAM",
    "MBC5+RAM+BATTERY",
    "MBC5+RUMBLE",
    "MBC5+RUMBLE+RAM",
    "MBC5+RUMBLE+RAM+BATTERY",
    "0x1F ???",
    "MBC6",
    "0x21 ???",
    "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
};
const c8* get_cartridge_type_name(u8 type)
{
    if (type <= 0x22) {
        return ROM_TYPES[type];
    }
    return "UNKNOWN";
}
static const c8* RAM_SIZE_TYPES[] = {
    "0",
    "-",
    "8 KB (1 bank)",
    "32 KB (4 banks of 8KB each)",
    "128 KB (16 banks of 8KB each)",
    "64 KB (8 banks of 8KB each)"
};
const c8* get_cartridge_ram_size_name(u8 ram_size_code)
{
    if(ram_size_code <= 0x05)
    {
        return RAM_SIZE_TYPES[ram_size_code];
    }
    return "UNKNOWN";
}
const c8* get_cartridge_lic_code_name(u8 lic_code)
{
    switch(lic_code)
    {
        case 0x00 : return "None";
        case 0x01 : return "Nintendo R&D1";
        case 0x08 : return "Capcom";
        case 0x13 : return "Electronic Arts";
        case 0x18 : return "Hudson Soft";
        case 0x19 : return "b-ai";
        case 0x20 : return "kss";
        case 0x22 : return "pow";
        case 0x24 : return "PCM Complete";
        case 0x25 : return "san-x";
        case 0x28 : return "Kemco Japan";
        case 0x29 : return "seta";
        case 0x30 : return "Viacom";
        case 0x31 : return "Nintendo";
        case 0x32 : return "Bandai";
        case 0x33 : return "Ocean/Acclaim";
        case 0x34 : return "Konami";
        case 0x35 : return "Hector";
        case 0x37 : return "Taito";
        case 0x38 : return "Hudson";
        case 0x39 : return "Banpresto";
        case 0x41 : return "Ubi Soft";
        case 0x42 : return "Atlus";
        case 0x44 : return "Malibu";
        case 0x46 : return "angel";
        case 0x47 : return "Bullet-Proof";
        case 0x49 : return "irem";
        case 0x50 : return "Absolute";
        case 0x51 : return "Acclaim";
        case 0x52 : return "Activision";
        case 0x53 : return "American sammy";
        case 0x54 : return "Konami";
        case 0x55 : return "Hi tech entertainment";
        case 0x56 : return "LJN";
        case 0x57 : return "Matchbox";
        case 0x58 : return "Mattel";
        case 0x59 : return "Milton Bradley";
        case 0x60 : return "Titus";
        case 0x61 : return "Virgin";
        case 0x64 : return "LucasArts";
        case 0x67 : return "Ocean";
        case 0x69 : return "Electronic Arts";
        case 0x70 : return "Infogrames";
        case 0x71 : return "Interplay";
        case 0x72 : return "Broderbund";
        case 0x73 : return "sculptured";
        case 0x75 : return "sci";
        case 0x78 : return "THQ";
        case 0x79 : return "Accolade";
        case 0x80 : return "misawa";
        case 0x83 : return "lozc";
        case 0x86 : return "Tokuma Shoten Intermedia";
        case 0x87 : return "Tsukuda Original";
        case 0x91 : return "Chunsoft";
        case 0x92 : return "Video system";
        case 0x93 : return "Ocean/Acclaim";
        case 0x95 : return "Varie";
        case 0x96 : return "Yonezawa/s’pal";
        case 0x97 : return "Kaneko";
        case 0x99 : return "Pack in soft";
        case 0xA4 : return "Konami (Yu-Gi-Oh!)";
        default: break;
    }
    return "UNKNOWN";
}
u8 mbc1_read(Emulator* emu, u16 addr)
{
    if(addr <= 0x3FFF)
    {
        if(emu->banking_mode && emu->num_rom_banks > 32)
        {
            usize bank_index = emu->ram_bank_number;
            usize bank_offset = bank_index * 32 * 16_kb;
            return emu->rom_data[bank_offset + addr];
        }
        else
        {
            return emu->rom_data[addr];
        }
    }
    if(addr >= 0x4000 && addr <= 0x7FFF)
    {
        // Cartridge ROM bank 01-7F.
        if(emu->banking_mode && emu->num_rom_banks > 32)
        {
            usize bank_index = emu->rom_bank_number + (emu->ram_bank_number << 5);
            usize bank_offset = bank_index * 16_kb;
            return emu->rom_data[bank_offset + (addr - 0x4000)];
        }
        else
        {
            usize bank_index = emu->rom_bank_number;
            usize bank_offset = bank_index * 16_kb;
            return emu->rom_data[bank_offset + (addr - 0x4000)];
        }
    }
    if(addr >= 0xA000 && addr <= 0xBFFF)
    {
        if(emu->cram)
        {
            if(!emu->cram_enable) return 0xFF;
            if(emu->num_rom_banks <= 32)
            {
                if(emu->banking_mode)
                {
                    // Advanced banking mode.
                    usize bank_offset = emu->ram_bank_number * 8_kb;
                    luassert(bank_offset + (addr - 0xA000) <= emu->cram_size);
                    return emu->cram[bank_offset + (addr - 0xA000)];
                }
                else
                {
                    // Simple banking mode.
                    return emu->cram[addr - 0xA000];
                }
            }
            else
            {
                // ram_bank_number is used for switching ROM banks, use 1 ram page.
                return emu->cram[addr - 0xA000];
            }
        }
    }
    log_error("LunaGB", "Unsupported bus read: %02X", (u32)addr);
    return 0xFF;
}
void mbc1_write(Emulator* emu, u16 addr, u8 data)
{
    if(addr <= 0x1FFF)
    {
        // Enable/disable cartridge RAM.
        if(emu->cram)
        {
            if((data & 0x0F) == 0x0A)
            {
                emu->cram_enable = true;
            }
            else
            {
                emu->cram_enable = false;
            }
            return;
        }
    }
    if(addr >= 0x2000 && addr <= 0x3FFF)
    {
        // Set ROM bank number.
        emu->rom_bank_number = data & 0x1F;
        if(emu->rom_bank_number == 0)
        {
            emu->rom_bank_number = 1;
        }
        if(emu->num_rom_banks <= 2)
        {
            emu->rom_bank_number = emu->rom_bank_number & 0x01;
        }
        else if(emu->num_rom_banks <= 4)
        {
            emu->rom_bank_number = emu->rom_bank_number & 0x03;
        }
        else if(emu->num_rom_banks <= 8)
        {
            emu->rom_bank_number = emu->rom_bank_number & 0x07;
        }
        else if(emu->num_rom_banks <= 16)
        {
            emu->rom_bank_number = emu->rom_bank_number & 0x0F;
        }
        return;
    }
    if(addr >= 0x4000 && addr <= 0x5FFF)
    {
        // Set RAM bank number.
        emu->ram_bank_number = data & 0x03;
        // Discards unsupported banks.
        if(emu->num_rom_banks > 32)
        {
            if(emu->num_rom_banks <= 64)
            {
                emu->ram_bank_number &= 0x01;
            }
        }
        else
        {
            if(emu->cram_size <= 8_kb)
            {
                emu->ram_bank_number = 0;
            }
            else if(emu->cram_size <= 16_kb)
            {
                emu->ram_bank_number &= 0x01;
            }
        }
        return;
    }
    if(addr >= 0x6000 && addr <= 0x7FFF)
    {
        // Set banking mode.
        if(emu->num_rom_banks > 32 || emu->cram_size > 8_kb)
        {
            emu->banking_mode = data & 0x01;
        }
        return;
    }
    if(addr >= 0xA000 && addr <= 0xBFFF)
    {
        if(emu->cram)
        {
            if(!emu->cram_enable) return;
            if(emu->num_rom_banks <= 32)
            {
                if(emu->banking_mode)
                {
                    // Advanced banking mode.
                    usize bank_offset = emu->ram_bank_number * 8_kb;
                    luassert(bank_offset + (addr - 0xA000) <= emu->cram_size);
                    emu->cram[bank_offset + (addr - 0xA000)] = data;
                }
                else
                {
                    // Simple banking mode.
                    emu->cram[addr - 0xA000] = data;
                }
            }
            else
            {
                // ram_bank_number is used for switching ROM banks, use 1 ram page.
                emu->cram[addr - 0xA000] = data;
            }
            return;
        }
    }
    log_error("LunaGB", "Unsupported bus write: %02X", (u32)addr);
}
u8 mbc2_read(Emulator* emu, u16 addr)
{
    if(addr <= 0x3FFF)
    {
        return emu->rom_data[addr];
    }
    if(addr >= 0x4000 && addr <= 0x7FFF)
    {
        // Cartridge ROM bank 01-0F.
        usize bank_index = emu->rom_bank_number;
        usize bank_offset = bank_index * 16_kb;
        return emu->rom_data[bank_offset + (addr - 0x4000)];
    }
    if(addr >= 0xA000 && addr <= 0xBFFF)
    {
        if(!emu->cram_enable) return 0xFF;
        u16 data_offset = addr - 0xA000;
        data_offset %= 512;
        return (emu->cram[data_offset] & 0x0F) | 0xF0;
    }
    log_error("LunaGB", "Unsupported bus read: %02X", (u32)addr);
    return 0xFF;
}
void mbc2_write(Emulator* emu, u16 addr, u8 data)
{
    if(addr <= 0x3FFF)
    {
        if(addr & 0x100) // bit 8 is set.
        {
            // Set ROM bank number.
            emu->rom_bank_number = data & 0x0F;
            if(emu->rom_bank_number == 0)
            {
                emu->rom_bank_number = 1;
            }
            if(emu->num_rom_banks <= 2)
            {
                emu->rom_bank_number = emu->rom_bank_number & 0x01;
            }
            else if(emu->num_rom_banks <= 4)
            {
                emu->rom_bank_number = emu->rom_bank_number & 0x03;
            }
            else if(emu->num_rom_banks <= 8)
            {
                emu->rom_bank_number = emu->rom_bank_number & 0x07;
            }
            return;
        }
        else
        {
            // Enable/disable cartridge RAM.
            if(emu->cram)
            {
                if(data == 0x0A)
                {
                    emu->cram_enable = true;
                }
                else
                {
                    emu->cram_enable = false;
                }
                return;
            }
        }
    }
    else if(addr >= 0xA000 && addr <= 0xBFFF)
    {
        if(!emu->cram_enable) return;
        u16 data_offset = addr - 0xA000;
        data_offset %= 512;
        emu->cram[data_offset] = data & 0x0F;
        return;
    }
    log_error("LunaGB", "Unsupported bus write: %02X", (u32)addr);
}
u8 mbc3_read(Emulator* emu, u16 addr)
{
    if(addr <= 0x3FFF)
    {
        return emu->rom_data[addr];
    }
    if(addr >= 0x4000 && addr <= 0x7FFF)
    {
        // Cartridge ROM bank 01-7F.
        usize bank_index = emu->rom_bank_number;
        usize bank_offset = bank_index * 16_kb;
        return emu->rom_data[bank_offset + (addr - 0x4000)];
    }
    if(addr >= 0xA000 && addr <= 0xBFFF)
    {
        if(emu->ram_bank_number <= 0x03)
        {
            if(emu->cram)
            {
                if(!emu->cram_enable) return 0xFF;
                usize bank_offset = emu->ram_bank_number * 8_kb;
                luassert(bank_offset + (addr - 0xA000) <= emu->cram_size);
                return emu->cram[bank_offset + (addr - 0xA000)];
            }
        }
        if(is_cart_timer(get_cartridge_header(emu->rom_data)->cartridge_type) && 
            emu->ram_bank_number >= 0x08 && emu->ram_bank_number <= 0x0C)
        {
            return ((u8*)(&emu->rtc.s))[emu->ram_bank_number - 0x08];
        }
    }
    log_error("LunaGB", "Unsupported bus read: %02X", (u32)addr);
    return 0xFF;
}
void mbc3_write(Emulator* emu, u16 addr, u8 data)
{
    if(addr <= 0x1FFF)
    {
        // Enable/disable cartridge RAM.
        if(data == 0x0A)
        {
            emu->cram_enable = true;
        }
        else
        {
            emu->cram_enable = false;
        }
        return;
    }
    if(addr >= 0x2000 && addr <= 0x3FFF)
    {
        // Set ROM bank number.
        emu->rom_bank_number = data & 0x7F;
        if(emu->rom_bank_number == 0)
        {
            emu->rom_bank_number = 1;
        }
        return;
    }
    if(addr >= 0x4000 && addr <= 0x5FFF)
    {
        if(data <= 0x03)
        {
            // Set RAM bank number.
            emu->ram_bank_number = data;
            return;
        }
        if(data >= 0x08 && data <= 0x0C)
        {
            // Map RTC registers.
            emu->ram_bank_number = data;
            return;
        }
    }
    if(addr >= 0x6000 && addr <= 0x7FFF)
    {
        if(data == 0x01 && emu->rtc.time_latching)
        {
            emu->rtc.latch();
        }
        if(data == 0x00)
        {
            emu->rtc.time_latching = true;
        }
        else
        {
            emu->rtc.time_latching = false;
        }
        return;
    }
    if(addr >= 0xA000 && addr <= 0xBFFF)
    {
        if(emu->ram_bank_number <= 0x03)
        {
            if(emu->cram)
            {
                if(!emu->cram_enable) return;
                if(emu->num_rom_banks <= 32)
                {
                    if(emu->banking_mode)
                    {
                        // Advanced banking mode.
                        usize bank_offset = emu->ram_bank_number * 8_kb;
                        luassert(bank_offset + (addr - 0xA000) <= emu->cram_size);
                        emu->cram[bank_offset + (addr - 0xA000)] = data;
                    }
                    else
                    {
                        // Simple banking mode.
                        emu->cram[addr - 0xA000] = data;
                    }
                }
                else
                {
                    // ram_bank_number is used for switching ROM banks, use 1 ram page.
                    emu->cram[addr - 0xA000] = data;
                }
            }
            return;
        }
        if(is_cart_timer(get_cartridge_header(emu->rom_data)->cartridge_type) && 
            emu->ram_bank_number >= 0x08 && emu->ram_bank_number <= 0x0C)
        {
            if(emu->ram_bank_number == 0x0C &&
                emu->rtc.halted() &&
                !bit_test(&data, 6))
            {
                emu->rtc.resume();
            }
            ((u8*)(&emu->rtc.s))[emu->ram_bank_number - 0x08] = data;
            emu->rtc.update_timestamp();
            return;
        }
    }
    log_error("LunaGB", "Unsupported bus write: %02X", (u32)addr);
}
u8 cartridge_read(Emulator* emu, u16 addr)
{
    u8 cartridge_type = get_cartridge_header(emu->rom_data)->cartridge_type;
    if(is_cart_mbc1(cartridge_type))
    {
        return mbc1_read(emu, addr);
    }
    else if(is_cart_mbc2(cartridge_type))
    {
        return mbc2_read(emu, addr);
    }
    else if(is_cart_mbc3(cartridge_type))
    {
        return mbc3_read(emu, addr);
    }
    else
    {
        if(addr <= 0x7FFF)
        {
            return emu->rom_data[addr];
        }
        if(addr >= 0xA000 && addr <= 0xBFFF && emu->cram)
        {
            return emu->cram[addr - 0xA000];
        }
    }
    log_error("LunaGB", "Unsupported cartridge read address: 0x%04X", (u32)addr);
    return 0xFF;
}
void cartridge_write(Emulator* emu, u16 addr, u8 data)
{
    u8 cartridge_type = get_cartridge_header(emu->rom_data)->cartridge_type;
    if(is_cart_mbc1(cartridge_type))
    {
        mbc1_write(emu, addr, data);
        return;
    }
    else if(is_cart_mbc2(cartridge_type))
    {
        mbc2_write(emu, addr, data);
        return;
    }
    else if(is_cart_mbc3(cartridge_type))
    {
        mbc3_write(emu, addr, data);
        return;
    }
    else
    {
        if(addr >= 0xA000 && addr <= 0xBFFF && emu->cram)
        {
            emu->cram[addr - 0xA000] = data;
            return;
        }
    }
    log_error("LunaGB", "Unsupported cartridge write address: 0x%04X", (u32)addr);
}