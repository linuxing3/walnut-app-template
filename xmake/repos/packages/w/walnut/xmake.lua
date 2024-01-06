package('walnut')
set_homepage('https://github.com/linuxing3/walnut-lib')
set_description('Bloat-free Immediate App Framework for C++ with minimal dependencies')
set_license('MIT')

add_urls('https://github.com/linuxing3/walnut-lib.git')
add_versions('v1.0.0', 'ad0189d05ce07737bfad16ad3f3373e8f2333029')

add_includedirs('include', 'include/Platform/GUI', 'include/Source')

on_install('macosx', 'linux', 'windows', 'mingw', 'android', 'iphoneos', function(package)
    import('package.tools.xmake').install(package, {})
    os.cp('Source', package:installdir('include'))
    os.cp('Platform', package:installdir('include'))
end)
