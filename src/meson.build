stdlib = static_library('pystd',
  'pystd_common.cpp',

  'pystd2025/common.cpp',
  'pystd2025/regex.cpp',
  'pystd2025/filesystem.cpp',
  'pystd2025/tables.cpp',
  'pystd2025/threading.cpp',

  'pystd2026/common.cpp',
  'pystd2026/argparse.cpp',
  'pystd2026/regex.cpp',
  'pystd2026/pathlib.cpp',
  'pystd2026/shutil.cpp',
  'pystd2026/tables.cpp',
  'pystd2026/tempfile.cpp',
  'pystd2026/threading.cpp',
  include_directories: '../include',
  dependencies: pcre_dep)

stdlib_dep = declare_dependency(include_directories: '../include',
  link_with: stdlib)
