--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

target('walnutapp')
set_kind('binary')
add_files('**.cpp')
add_includedirs('.')
add_packages('walnut', 'glfw-walnut', 'vulkan-headers', 'imgui-walnut')
add_links('glfw-walnut', 'walnut')
