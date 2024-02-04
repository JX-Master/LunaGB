#pragma once
#include <Luna/Runtime/Result.hpp>
#include "CPU.hpp"
using namespace Luna;

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