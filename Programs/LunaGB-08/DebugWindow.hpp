#pragma once
#include <Luna/Runtime/Vector.hpp>
#include <Luna/Runtime/String.hpp>
#include <Luna/Runtime/Ref.hpp>
#include <Luna/RHI/Texture.hpp>
using namespace Luna;

struct DebugWindow
{
    bool show = false;

    // CPU log.
    String cpu_log;
    bool cpu_logging = false;

    // Serial inspector.
    Vector<u8> serial_data;

    // Tiles inspector
    Ref<RHI::ITexture> tile_texture;

    void gui();
    void cpu_gui();
    void serial_gui();
    void tiles_gui();
    void ppu_gui();
};