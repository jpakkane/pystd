project('pystd', 'cpp', default_options : ['cpp_std=c++23'])

add_global_arguments('-nostdinc++', language: 'cpp')
add_global_link_arguments('-nostdlib++', language: 'cpp')

executable('pystd', 'pystd.cpp', 'main.cpp')
