#pragma once
#include <Luna/RHI/Device.hpp>
#include "Emulator.hpp"
#include <Luna/Runtime/UniquePtr.hpp>
using namespace Luna;

struct App
{
    //! `true` if the application is exiting. For example, if the user presses the close
    //! button of the window.
    bool is_exiting;

    //! The RHI device used to render interfaces.
    Ref<RHI::IDevice> rhi_device;
    //! The graphics queue index.
    u32 rhi_queue_index;

    //! The application main window.
    Ref<Window::IWindow> window;
    //! The application main window swap chain.
    Ref<RHI::ISwapChain> swap_chain;
    //! The command buffer used to submit draw calls.
    Ref<RHI::ICommandBuffer> cmdbuf;

    //! The emulator instance.
    UniquePtr<Emulator> emulator;
    //! The ticks for last frame.
    u64 last_frame_ticks;

    RV init();
    RV update();
    ~App();

    void draw_gui();
    void draw_main_menu_bar();

    void open_cartridge();
    void close_cartridge();
};