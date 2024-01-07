--- @diagnostic disable:undefined-global
package('glfw-walnut')

set_homepage('https://github.com/TheCherno/glfw')
set_description(
    'GLFW is an Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan application development.'
)
set_license('zlib')

add_urls('https://github.com/TheCherno/glfw.git')
add_versions('walnut', 'c186bef662454c9a11a19e0773a8a7c07bd0aa47')
add_configs('glfw_include', {
    description = 'Choose submodules enabled in glfw',
    default = 'none',
    type = 'string',
    values = { 'none', 'vulkan', 'glu', 'glext', 'es2', 'es3' },
})

if is_plat('linux') then
    add_extsources('apt::libglfw3-dev', 'pacman::glfw-x11')
end

add_deps('cmake')
add_deps('opengl', { optional = true })
if is_plat('macosx') then
    add_frameworks('Cocoa', 'IOKit')
elseif is_plat('windows') then
    add_syslinks('user32', 'shell32', 'gdi32')
elseif is_plat('mingw') then
    add_syslinks('gdi32')
elseif is_plat('linux') then
    -- TODO: add wayland support
    add_deps('libx11', 'libxrandr', 'libxrender', 'libxinerama', 'libxfixes', 'libxcursor', 'libxi', 'libxext')
    add_syslinks('dl', 'pthread')
    add_defines('_GLFW_X11')
end

on_load(function(package)
    package:add('defines', 'GLFW_INCLUDE_' .. package:config('glfw_include'):upper())
end)

on_install('macosx', 'windows', 'linux', 'mingw', function(package)
    os.cp('include/GLFW', package:installdir('include'))
    os.cp(path.join(package:scriptdir(), 'port', 'xmake.lua'), 'xmake.lua')
    import('package.tools.xmake').install(package, configs)
end)

on_test(function(package)
    assert(package:check_csnippets({
        test = [[
            void test() {
                glfwInit();;
            }
        ]],
    }, { configs = { languages = 'c11' }, includes = 'GLFW/glfw3.h' }))
end)
