project('pystd', 'cpp', default_options : ['cpp_std=c++26'])

add_project_arguments('-nostdinc++', language: 'cpp')
add_project_link_arguments('-nostdlib++', '-lsupc++', language: 'cpp')

pcre_dep = dependency('libpcre2-8')

subdir('src')

executable('pystd', 'main.cpp',
  dependencies: stdlib_dep)

executable('maptime', 'maptime.cpp',
  dependencies: stdlib_dep)

subdir('test')
subdir('tools')
