add_rules('mode.debug', 'mode.release')

if is_os('windows') then
    target('glfw-walnut')
    set_languages('c++20')
    set_kind('static')
    add_defines('_GLFW_WIN32', '_CRT_SECURE_NO_WARNINGS')
    add_files(
        'src/context.c',
        'src/init.c',
        'src/input.c',
        'src/monitor.c',
        'src/null_init.c',
        'src/null_joystick.c',
        'src/null_monitor.c',
        'src/null_window.c',
        'src/win32_platform.c',
        'src/vulkan.c',
        'src/window.c'
    )
    add_files(
        'src/win32_init.c',
        'src/win32_joystick.c',
        'src/win32_module.c',
        'src/win32_monitor.c',
        'src/win32_time.c',
        'src/win32_thread.c',
        'src/win32_window.c',
        'src/wgl_context.c',
        'src/egl_context.c',
        'src/osmesa_context.c'
    )
    add_includedirs('include')
    add_links('gdi32', 'shell32')
else
    target('glfw-walnut')
    set_languages('c++20')
    set_kind('static')
    add_defines('_GLFW_X11')
    add_files(
        'src/context.c',
        'src/init.c',
        'src/input.c',
        'src/monitor.c',
        'src/null_init.c',
        'src/null_joystick.c',
        'src/null_monitor.c',
        'src/null_window.c',
        'src/vulkan.c',
        'src/platform.c',
        'src/window.c'
    )
    add_files(
        'src/x11_init.c',
        'src/x11_monitor.c',
        'src/x11_window.c',
        'src/x11_platform.c',
        'src/xkb_unicode.c',
        'src/posix_module.c',
        'src/posix_time.c',
        'src/posix_thread.c',
        'src/posix_module.c',
        'src/glx_context.c',
        'src/egl_context.c',
        'src/osmesa_context.c',
        'src/linux_joystick.c'
    )
    add_includedirs('include')
    add_links('libx11', 'libxrandr', 'libxrender', 'libxinerama', 'libxfixes', 'libxcursor', 'libxi', 'libxext')
    add_syslinks('dl', 'pthread')
    add_defines('_GLFW_X11')
end
