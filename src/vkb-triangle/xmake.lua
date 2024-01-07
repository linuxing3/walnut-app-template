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
    os.cp('$(scriptdir)/resources/shaders/*.spv', target:targetdir())
end)
-- add_includedirs(path.join(os.getenv('HOME'), 'Downloads/vulkan_sdk/x86_64/include/'))
-- add_linkdirs(path.join(os.getenv('HOME'), 'Downloads/vulkan_sdk/x86_64/lib'))
-- add_links('vulkan')
