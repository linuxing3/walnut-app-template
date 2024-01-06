add_requires('fmt')
add_requires('vk-bootstrap')

add_requires('imgui v1.90-docking', { configs = { glfw = true, vulkan = true } })
add_requires('glfw', { configs = { glfw_include = 'vulkan' } })
add_requires('glm')
add_requires('stb')
add_requires('spdlog')
add_requires('vulkan-headers')
add_requires('walnut')

if has_config('test') then
    add_requires('gtest')
end
