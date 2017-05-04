# TODO: version, tests, valgrind, visibility, config generation

project(
    'mini-async-log-c',
    'c',
    version : '0.0.1',
    license : 'BSD-3',
    default_options : [
        'c_std=gnu11',
#        'warning_level=1',
        'default_library=static'
        ]
    )
version_minor = '0'
version_major = '0'
version_rev   = '1'
version       = version_minor + '.' + version_major + '.' + version_rev

base_library = subproject(
    'base_library',
    default_options : ['timestamp_64bit=true']
    )
base_lib          = base_library.get_variable ('base_lib')
nonblock_lib      = base_library.get_variable ('nonblock_lib')
libcmocka_path    = base_library.get_variable ('libcmocka_path')
build_helpers_dir = base_library.get_variable ('build_helpers_dir')

include_dirs = [
    include_directories ('include'),
    include_directories ('src'),
]
include_dirs += base_library.get_variable ('include_dirs')

test_include_dirs  = include_dirs
test_include_dirs += [ include_directories ('test/src') ]
test_include_dirs +=  base_library.get_variable ('test_include_dirs')

pkg_mod = import ('pkgconfig')
install_subdir ('include/malc', install_dir : 'include')

cflags = base_library.get_variable ('cflags')
lib_cflags = []
test_cflags = []
test_link_args = [ libcmocka_path ]

if get_option ('sharedlib')
    libtype = 'shared_library'
else
    libtype = 'static_library'
endif

cc = meson.get_compiler ('c')
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
    if get_option ('pic_statlibs') and libtype == 'static_library'
        lib_cflags += ['-fPIC']
    endif
endif

host_system = host_machine.system()
if host_system == 'windows'
    test_link_args += ['-lwinmm.lib'] # Untested
endif

threads = dependency ('threads')

cdata = configuration_data()
if get_option ('no_compressed_builtins')
    cflags += [ '-DMALC_NO_BUILTIN_COMPRESSION' ]
    cdata.set(
        'no_compressed_builtins_placeholder',
        '#define MALC_NO_BUILTIN_COMPRESSION'
        )
endif

if get_option ('no_compressed_ptrs')
    cflags += [ '-DMALC_NO_PTR_COMPRESSION' ]
    cdata.set(
        'no_compressed_ptrs_placeholder', '#define MALC_NO_PTR_COMPRESSION'
        )
endif

cdata.set ('version', version)
cdata.set ('version_major', version_major)
cdata.set ('version_minor', version_minor)
cdata.set ('version_rev', version_rev)

configure_file(
    input:         'config.h.in',
    output:        'config.h',
    configuration: cdata,
    install_dir:   'include/malc'
    )

#- malc ------------------------------------------------------------------------

malc_srcs = [
    'src/malc/memory.c',
    'src/malc/malc.c',
    'src/malc/tls_buffer.c',
    'src/malc/serialization.c',
]
malc_test_srcs = [
    'test/src/malc/tests_main.c',
    'test/src/malc/tls_buffer_test.c',
    'test/src/malc/interface_test.c',
    'test/src/malc/serialization_test.c',
]

#cflags += [ '-E' ] #erase

malc_lib = build_target(
    'malc',
    malc_srcs,
    target_type         : libtype,
    include_directories : include_dirs,
    link_with           : [ base_lib, nonblock_lib ],
    c_args              : cflags + lib_cflags,
    install             : true
    )
pkg_mod.generate(
    libraries   : malc_lib,
    version     : '0.0.1',
    name        : 'libmalc',
    filebase    : 'malc',
    description : 'simulator task scheduler'
    )
test(
    'malc',
    executable(
        'malc-test',
        malc_test_srcs,
        include_directories : test_include_dirs,
        link_with           : malc_lib,
        link_args           : test_link_args,
        c_args              : cflags + test_cflags,
        dependencies        : threads
    ))
