project('pystd', 'cpp', default_options : ['cpp_std=c++23'])

add_global_arguments('-nostdinc++', language: 'cpp')
add_global_link_arguments('-nostdlib++', '-lsupc++', language: 'cpp')

pcre_dep = dependency('libpcre2-8')

stdlib = library('pystd', 'pystd2025.cpp', 'pystd2025_regex.cpp',
  dependencies: pcre_dep)

stdlib_dep = declare_dependency(include_directories: '.',
  link_with: stdlib)

executable('pystd', 'main.cpp',
  dependencies: stdlib_dep)

executable('pystd2055test', 'pystd2025test.cpp',
  dependencies: stdlib_dep)
