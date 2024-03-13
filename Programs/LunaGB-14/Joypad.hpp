#pragma once
#include <Luna/Runtime/MemoryUtils.hpp>
using namespace Luna;

struct Emulator;
struct Joypad
{
    bool a;
    bool b;
    bool select;
    bool start;
    bool right;
    bool left;
    bool up;
    bool down;
    //! 0xFF00
    u8 p1;

    void init();
    u8 get_key_state() const;
    void update(Emulator* emu);
    u8 bus_read();
    void bus_write(u8 v);
};