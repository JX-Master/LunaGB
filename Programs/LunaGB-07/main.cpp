#include "App.hpp"
#include <Luna/Runtime/Runtime.hpp>
#include <Luna/Runtime/Module.hpp>
#include <Luna/Runtime/Log.hpp>

// For module interfaces.
#include <Luna/Window/Window.hpp>
#include <Luna/RHI/RHI.hpp>
#include <Luna/AHI/AHI.hpp>
#include <Luna/ImGui/ImGui.hpp>

App* g_app;

RV run_app()
{
    lutry
    {
        // Add modules.
        luexp(add_modules({module_window(), module_rhi(), module_ahi(), module_imgui()}));
        // Initialize modules.
        luexp(init_modules());
        // Run the application.
        luexp(g_app->init());
        while(!g_app->is_exiting)
        {
            luexp(g_app->update());
        }
    }
    lucatchret;
    return ok;
}

int main()
{
    bool inited = Luna::init();
    if(!inited) return -1;
    g_app = memnew<App>();
    RV r = run_app();
    if(failed(r))
    {
        log_error("LunaGB", explain(r.errcode()));
    }
    memdelete(g_app);
    Luna::close();
    return 0;
}