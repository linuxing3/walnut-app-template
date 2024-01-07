package('wgpu-dist')
set_homepage('https://github.com/eliemichel/WebGPU-distribution.git')
set_description('wgpu native distribution')
set_license('MIT')

add_urls('https://github.com/eliemichel/WebGPU-distribution.git')
add_versions('wgpu-static', '24de126076f993d047629ce94f87bdbecfd44e17')
add_versions('wgpu', '9dd47f8515dfd7112b750da07e719460a88bf2e8')

add_includedirs('include', 'include/webgpu')

on_install('macosx', 'linux', 'windows', 'mingw', 'android', 'iphoneos', function(package)
    os.cp('include/webgpu', package:installdir('include'))
    if package:is_plat('linux') then
        local from = path.join('bin', package:plat() .. '-' .. package:arch(), 'libwgpu_native.so')
        os.cp(from, package:installdir('bin'))
    elseif package:is_plat('windows') then
        local from = path.join('bin', package:plat() .. '_' .. package:arch(), 'libwgpu_native.lib')
        os.cp(from, package:installdir('lib'))
    end
end)

on_test(function(package)
    assert(package:has_cfuncs('wgpuCreateInstance', { includes = 'wgpu.h' }))
end)
