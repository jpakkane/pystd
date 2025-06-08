stdlib = static_library('pystd',
  'pystd2025.cpp',
  'pystd2025_regex.cpp',
  'pystd2025_filesystem.cpp',
  include_directories: '../include',
  dependencies: pcre_dep)

stdlib_dep = declare_dependency(include_directories: '../include',
  link_with: stdlib)
