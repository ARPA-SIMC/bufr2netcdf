# Generate config.h
configure_file(output: 'config.h', configuration: conf_data)

sources = [
    'utils.cc',
    'mnemo.cc',
    'namer.cc',
    'valarray.cc',
    'plan.cc',
    'arrays.cc',
    'ncoutfile.cc',
    'convert.cc',
]

bufr2netcdf = executable('bufr2netcdf', sources + ['bufr2netcdf.cc'], 
    dependencies: [libwreport_dep, netcdf_dep],
    install: true,
)

nccmp_sources = [
    'nccmp/getopt.c',
    'nccmp/nccmp.c',
    'nccmp/ncinfo.c',
    'nccmp/opt.c',
    'nccmp/strlist.c',
    'nccmp/xmalloc.c',
]

nccmp = executable('nccmp', nccmp_sources,
    dependencies: [netcdf_dep],
    install: false,
    c_args: ['-DVERSION="1.4.0"', '-w'],
)

test_sources = [
    'mnemo-test.cc',
    'namer-test.cc',
    'ncoutfile-test.cc',
    'valarray-test.cc',
    'plan-test.cc',
    'arrays-test.cc',
    'convert-test.cc',
    'tests/tests.cc',
    'tests/tests-main.cc',
]

test_bufr2netcdf = executable('test_bufr2netcdf', sources + test_sources,
    dependencies: [
        libwreport_dep,
        netcdf_dep,
    ])

runtest = find_program('../runtest')

test('bufr2netcdf', runtest, args: [test_bufr2netcdf], env:['B2NC_NCCMP=' + nccmp.full_path()], depends: [nccmp], timeout: 600)
