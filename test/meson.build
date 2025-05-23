cdata = configuration_data()
cdata.set_quoted('TESTDATADIR', meson.current_source_dir())
configure_file(input: 'pystd_testconfig.hpp.in',
  output: 'pystd_testconfig.hpp',
  configuration: cdata)

pystd2025_test = executable('pystd2025test', 'pystd2025test.cpp',
  dependencies: stdlib_dep)

test('pystd2025', pystd2025_test)
