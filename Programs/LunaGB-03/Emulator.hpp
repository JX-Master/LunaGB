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

    RV init(const void* cartridge_data, usize cartridge_data_size);
    void update(f64 delta_time);
    void close();
    ~Emulator()
    {
        close();
    }

    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);

    void tick_cpu();
};