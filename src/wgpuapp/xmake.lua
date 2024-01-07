--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

target('wgpuapp')
set_languages('c++20')
set_kind('binary')
add_files('**.cpp')
add_includedirs('.')
add_packages('glfw3webgpu', 'wgpu-native', 'glfw-walnut', 'vulkan-headers')
add_packages('imgui-walnut')
after_build(function(target) end)
