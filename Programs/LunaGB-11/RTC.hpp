#pragma once
#include <Luna/Runtime/MemoryUtils.hpp>
using namespace Luna;

struct RTC
{
    u8 s; // Seconds.
    u8 m; // Minutes.
    u8 h; // Hours.
    u8 dl;// Lower 8 bits of Day Counter.
    u8 dh;// Upper 1 bit of Day Counter, Carry Bit, Halt Flag.

    // Internal state.
    f64 time;
    bool time_latched;
    // Set to `true` when writing 0x00 to 0x6000~0x7FFF.
    bool time_latching;

    void init();
    void update(f64 delta_time);
    void update_time_registers();
    void update_timestamp();
    void latch();
    u16 days() const { return (u16)dl + (((u16)(dh & 0x01)) << 8); }
    bool halted() const { return bit_test(&dh, 6); }
    bool day_overflow() const { return bit_test(&dh, 7); }
};