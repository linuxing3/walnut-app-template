add_requires('fmt')
add_requires('vk-bootstrap', 'glfw', 'vulkan-headers')

if has_config('test') then
    add_requires('gtest')
end
