#pragma once
#include <Luna/Runtime/Result.hpp>
#include "CPU.hpp"
#include "Timer.hpp"
#include "Serial.hpp"
#include "PPU.hpp"
#include "Joypad.hpp"
#include <Luna/Runtime/Path.hpp>
using namespace Luna;

constexpr u8 INT_VBLANK = 1;
constexpr u8 INT_LCD_STAT = 2;
constexpr u8 INT_TIMER = 4;
constexpr u8 INT_SERIAL = 8;
constexpr u8 INT_JOYPAD = 16;

struct Emulator
{
    //! The cartridge file path. Used for saving cartridge RAM data if any.
    Path cartridge_path;

    byte_t* rom_data = nullptr;
    usize rom_data_size = 0;

    //! The cartridge RAM.
    byte_t* cram = nullptr;
    //! The cartridge RAM size. 
    usize cram_size = 0;

    //! The number of ROM banks. 16KB per bank.
    usize num_rom_banks = 0;
    //! MBC1/MBC2: The cartridge RAM is enabled for reading / writing.
    bool cram_enable = false;
    //! MBC1/MBC2: The ROM bank number controlling which rom bank is mapped to 0x4000~0x7FFF.
    u8 rom_bank_number = 1;
    //! MBC1: The RAM bank number register controlling which ram bank is mapped to 0xA000~0xBFFF.
    //! If the cartridge ROM size is larger than 512KB (32 banks), this is used to control the 
    //! high 2 bits of rom bank number, enabling the game to use at most 2MB of ROM data.
    u8 ram_bank_number = 0;
    //! MBC1: The banking mode.
    //! 0: 0000–3FFF and A000–BFFF are locked to bank 0 of ROM and SRAM respectively.
    //! 1: 0000–3FFF and A000-BFFF can be bank-switched via the 4000–5FFF register.
    u8 banking_mode = 0;

    //! `true` if the emulation is paused.
    bool paused = false;
    //! The cycles counter.
    u64 clock_cycles = 0;
    //! The clock speed scale value.
    f32 clock_speed_scale = 1.0;

    CPU cpu;

    byte_t vram[8_kb];
    byte_t wram[8_kb];
    byte_t oam[160];
    byte_t hram[128];

    //! 0xFF0F - The interruption flags.
    u8 int_flags;
    //! 0xFFFF - The interruption enabling flags.
    u8 int_enable_flags;

    Timer timer;
    Serial serial;
    PPU ppu;
    Joypad joypad;

    RV init(Path cartridge_path, const void* cartridge_data, usize cartridge_data_size);
    void update(f64 delta_time);
    //! Advances clock and updates all hardware states (except CPU).
    //! This is called from CPU instructions.
    //! @param[in] mcycles The number of machine cycles to tick.
    void tick(u32 mcycles);
    void close();
    ~Emulator()
    {
        close();
    }

    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);
    void load_cartridge_ram_data();
    void save_cartridge_ram_data();
};