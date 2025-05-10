project('bufr2netcdf', 'c', 'cpp', version: '1.8', license : 'GPL-2.0-or-later', default_options: ['warning_level=3', 'cpp_std=c++17'], meson_version: '>= 0.58')
# TODO: use warning_level=everything in meson from 1.0

cpp = meson.get_compiler('cpp')

warning_control = [
  # Turn some warning classes to errors
  '-Werror=format',
  '-Werror=suggest-override',
  '-Werror=deprecated-copy-dtor',
  '-Werror=missing-declarations',
  '-Werror=overloaded-virtual',
  '-Werror=cast-qual',                
  '-Werror=duplicated-branches',
  '-Werror=logical-op',
  '-Werror=catch-value',
  '-Werror=conditionally-supported',   
  '-Werror=c++23-extensions',
  '-Werror=dangling-else',
  '-Werror=suggest-attribute=format',
  # '-Werror=deprecated-declarations',
  # '-Werror=noexcept',
  '-Werror=cast-align',

  #  '-Wno-padded',
  #  '-Wno-abi-tag',
  '-Wswitch',
  #  '-Wno-switch-enum',
  #  '-Wno-effc++',
  #
  # These ones can be activated from time to time
  # '-Wfloat-equal',
  '-Wsuggest-attribute=noreturn',
  '-Wformat-truncation',
  '-Warith-conversion',
  # '-Wconversion',
]

add_project_arguments(
  cpp.get_supported_arguments(warning_control),
  language : 'cpp')

table_dir = get_option('datadir') / 'bufr2netcdf'

conf_data = configuration_data()
conf_data.set_quoted('table_dir', get_option('prefix') / table_dir)

if cpp.has_function('getopt_long', prefix : '#include <getopt.h>')
  conf_data.set('HAS_GETOPT_LONG', 1)
endif

toplevel_inc = include_directories('.')

# Dependencies
libwreport_dep = dependency('libwreport', version: '>= 3.38')
netcdf_dep = dependency('netcdf')

# Generate the builddir's version of run-local
run_local_cfg = configure_file(output: 'run-local', input: 'run-local.in', configuration: {
    'top_srcdir': meson.project_source_root(),
    'top_builddir': meson.project_build_root(),
})
# Just using the configure_file object in a custom_target command gives:
#   'AttributeError: 'File' object has no attribute 'replace'
# Using find_program on the resulting file works around that
# See https://github.com/mesonbuild/meson/issues/8039
run_local = find_program(run_local_cfg)

subdir('src')

mnem_tables = files(
    'tables/mnem_001',
    'tables/mnem_002',
    'tables/mnem_003',
    'tables/mnem_004',
    'tables/mnem_005',
    'tables/mnem_006',
    'tables/mnem_007',
    'tables/mnem_008',
    'tables/mnem_009',
    'tables/mnem_010',
    'tables/mnem_011',
    'tables/mnem_012',
    'tables/mnem_013',
    'tables/mnem_014',
    'tables/mnem_015',
    'tables/mnem_016',
    'tables/mnem_017',
    'tables/mnem_018',
    'tables/mnem_019',
    'tables/mnem_020',
    'tables/mnem_021',
    'tables/mnem_022',
    'tables/mnem_023',
    'tables/mnem_024',
    'tables/mnem_025',
    'tables/mnem_026',
    'tables/mnem_027',
    'tables/mnem_028',
    'tables/mnem_029',
    'tables/mnem_030',
    'tables/mnem_031',
    'tables/mnem_032',
    'tables/mnem_033',
    'tables/mnem_034',
)

install_data(mnem_tables, install_dir: table_dir)
