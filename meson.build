project('pystd', 'cpp', default_options : ['cpp_std=c++26'])

add_project_arguments('-nostdinc++', language: 'cpp')
if host_machine.system() == 'darwin'
  # If someone knows the macOS equivalent of -lsupc++,
  # submit a patch. Until then we have to link against
  # the main C++ standard library.
  #add_project_link_arguments('-nostdlib++',  language: 'cpp')
else
  add_project_link_arguments('-nostdlib++', '-lsupc++', language: 'cpp')
endif

pcre_dep = dependency('libpcre2-8')

subdir('src')

executable('pystd', 'main.cpp',
  dependencies: stdlib_dep)

subdir('test')
subdir('tools')
