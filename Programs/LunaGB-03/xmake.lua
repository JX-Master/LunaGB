target("LunaGB-03")
    set_luna_sdk_program()
    add_headerfiles("**.hpp")
    add_files("**.cpp")
    add_deps("Runtime", "Window", "RHI", "ShaderCompiler", "ImGui", "HID", "AHI")
target_end()