#pragma once
#include <Luna/Runtime/Vector.hpp>
#include <Luna/Runtime/String.hpp>
using namespace Luna;

struct DebugWindow
{
    bool show = false;

    // CPU log.
    String cpu_log;
    bool cpu_logging = false;

    // Serial inspector.
    Vector<u8> serial_data;

    void gui();
    void cpu_gui();
    void serial_gui();
};