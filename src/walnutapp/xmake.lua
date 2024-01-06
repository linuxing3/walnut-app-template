--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

target('walnutapp')
set_kind('binary')
add_files('**.cpp')
add_includedirs('.')
add_packages('vk-bootstrap', 'glfw', 'vulkan-headers', 'imgui')
add_packages('walnut')
add_links('glfw')
add_syslinks('dl')
add_includedirs(path.join(os.getenv('HOME'), 'Downloads/vulkan_sdk/x86_64/include/'))
add_linkdirs(path.join(os.getenv('HOME'), 'Downloads/vulkan_sdk/x86_64/lib'))
add_links('vulkan')
