cdata = configuration_data()
cdata.set_quoted('TESTDATADIR', meson.current_source_dir())
configure_file(input: 'pystd_testconfig.hpp.in',
  output: 'pystd_testconfig.hpp',
  configuration: cdata)

pystd2025_test = executable('pystd2025test', 'pystd2025test.cpp',
  dependencies: stdlib_dep)

test('pystd2025', pystd2025_test)

pystd2026_test = executable('pystd2026test', 'pystd2026test.cpp',
  dependencies: stdlib_dep)

test('pystd2026', pystd2026_test)

pystd2026_test_regex = executable('pystd2026test_regex', 'pystd2026test_regex.cpp',
  dependencies: stdlib_dep)

test('pystd2026_regex', pystd2026_test_regex)

pystd2026_test_btree = executable('pystd2026test_btree', 'pystd2026test_btree.cpp',
  dependencies: stdlib_dep)

test('pystd2026_btree', pystd2026_test_btree)

pystd2026_test_threading = executable('pystd2026test_threading', 'pystd2026test_threading.cpp',
  dependencies: stdlib_dep)

test('pystd2026_threading', pystd2026_test_threading)
