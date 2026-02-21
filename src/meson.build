stdlib = static_library('pystd',
  'pystd_common.cpp',
  'pystd2025.cpp',
  'pystd2025_regex.cpp',
  'pystd2025_filesystem.cpp',
  'pystd2025_tables.cpp',
  'pystd2025_threading.cpp',
  'pystd2026.cpp',
  'pystd2026_argparse.cpp',
  'pystd2026_regex.cpp',
  'pystd2026_filesystem.cpp',
  'pystd2026_tables.cpp',
  'pystd2026_threading.cpp',
  include_directories: '../include',
  dependencies: pcre_dep)

stdlib_dep = declare_dependency(include_directories: '../include',
  link_with: stdlib)
