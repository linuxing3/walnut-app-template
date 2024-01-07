package('walnut')
set_homepage('https://github.com/linuxing3/walnut-lib')
set_description('Bloat-free Immediate App Framework for C++ with minimal dependencies')
set_license('MIT')

add_urls('https://github.com/linuxing3/walnut-lib.git')
add_versions('v1.0.0', '3186f2f61812cd50f33a3bd91c7a1fc64454a025')

add_includedirs('include', 'include/Platform/GUI', 'include/Source')

on_install('macosx', 'linux', 'windows', 'mingw', 'android', 'iphoneos', function(package)
    os.cp('Source', package:installdir('include'))
    os.cp('Platform', package:installdir('include'))
    os.cp(path.join(package:scriptdir(), 'port', 'xmake.lua'), 'xmake.lua')
    import('package.tools.xmake').install(package, {})
end)
