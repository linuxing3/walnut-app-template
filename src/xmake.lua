--- @diagnostic disable:undefined-global

add_rules('utils.install.cmake_importfiles')
add_rules('utils.install.pkgconfig_importfiles')

-- 保留头文件目录结构
add_headerfiles('src/include/(**.h)')
-- 丢弃文件目录结构，所有头文件都放在 include 目录
-- add_headerfiles('include/**.h')

includes('vkb-triangle')
includes('walnutapp')
includes('wgpuapp')
