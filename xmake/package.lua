add_requires('fmt')
add_requires('vk-bootstrap')

add_requires('imgui-walnut walnut', { configs = { glfw = true, vulkan = true } })
add_requires('glfw-walnut walnut', { configs = { glfw_include = 'vulkan' } })
add_requires('glm')
add_requires('stb')
add_requires('tinyobjloader')
add_requires('spdlog')
add_requires('vulkan-headers')
add_requires('walnut')

add_requires('glfw3webgpu')
add_requires('wgpu-dist wgpu')

if has_config('test') then
    add_requires('gtest')
end
