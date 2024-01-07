--- @diagnostic disable:undefined-global

add_rules('mode.debug', 'mode.release')

target('vk-app')
set_languages('c++14')
set_kind('binary')
add_files('**.cpp')
add_includedirs('.')
add_packages('vk-bootstrap', 'glfw-walnut', 'vulkan-headers')
add_syslinks('dl')
after_build(function(target)
    os.cp('$(projectdir)/resources/shaders/*.spv', target:targetdir())
end)
