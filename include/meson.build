cdata = configuration_data()

cdata.set('PYSTD2025_HAS_REGEX', pcre_dep.found())
cdata.set('PYSTD2026_HAS_REGEX', pcre_dep.found())

configure_file(input: 'pystd_config.hpp.in',
  output: 'pystd_config.hpp',
  configuration: cdata)
