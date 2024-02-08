#pragma once
#include <Luna/Runtime/Result.hpp>
#include "CPU.hpp"
#include "Timer.hpp"
#include "Serial.hpp"
#include "PPU.hpp"
using namespace Luna;

constexpr u8 INT_VBLANK = 1;
constexpr u8 INT_LCD_STAT = 2;
constexpr u8 INT_TIMER = 4;
constexpr u8 INT_SERIAL = 8;
constexpr u8 INT_JOYPAD = 16;

struct Emulator
{
    byte_t* rom_data = nullptr;
    usize rom_data_size = 0;

    //! `true` if the emulation is paused.
    bool paused = false;
    //! The cycles counter.
    u64 clock_cycles = 0;
    //! The clock speed scale value.
    f32 clock_speed_scale = 1.0;

    CPU cpu;

    byte_t vram[8_kb];
    byte_t wram[8_kb];
    byte_t hram[128];

    //! 0xFF0F - The interruption flags.
    u8 int_flags;
    //! 0xFFFF - The interruption enabling flags.
    u8 int_enable_flags;

    Timer timer;
    Serial serial;
    PPU ppu;

    RV init(const void* cartridge_data, usize cartridge_data_size);
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
};