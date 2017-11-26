# TODO: version, tests, valgrind, visibility, config generation

project(
    'mini-async-log-c',
    ['c', 'cpp'],
    version : '0.0.1',
    license : 'BSD-3',
    default_options : [
        'c_std=gnu11',
        'cpp_std=c++11',
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

smoke_test_include_dirs  = include_dirs
smoke_test_include_dirs += [ include_directories ('smoke_test/src') ]
smoke_test_include_dirs +=  base_library.get_variable ('test_include_dirs')

pkg_mod = import ('pkgconfig')
install_subdir ('include/malc', install_dir : 'include')

cflags = base_library.get_variable ('cflags')
cflags += [ '-DMALC_LIBRARY_COMPILATION' ]
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
    ncbph_str = '#define MALC_NO_BUILTIN_COMPRESSION 1'
else
    ncbph_str = ''
endif
cdata.set('no_compressed_builtins_placeholder', ncbph_str)

if get_option ('no_compressed_ptrs')
    cflags += [ '-DMALC_NO_PTR_COMPRESSION' ]
    ncptrs_str = '#define MALC_NO_PTR_COMPRESSION 1'
else
    ncptrs_str = ''
endif
cdata.set('no_compressed_ptrs_placeholder', ncptrs_str)

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
    'src/malc/bounded_buffer.c',
    'src/malc/serialization.c',
    'src/malc/entry_parser.c',
    'src/malc/destinations.c',
    'src/malc/destinations/array.c',
    'src/malc/destinations/stdouterr.c',
    'src/malc/destinations/file.c',
]
malc_test_srcs = [
    'test/src/malc/tests_main.c',
    'test/src/malc/tls_buffer_test.c',
    'test/src/malc/bounded_buffer_test.c',
    'test/src/malc/interface_test.c',
    'test/src/malc/serialization_test.c',
    'test/src/malc/entry_parser_test.c',
    'test/src/malc/destinations_test.c',
    'test/src/malc/array_destination_test.c',
    'test/src/malc/file_destination_test.c',
]
malc_smoke_test_srcs = [
    'smoke_test/src/malc/smoke.c',
]

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
    description : 'mini-async-log-c'
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
test(
    'malc',
    executable(
        'malc-smoke',
        malc_smoke_test_srcs,
        include_directories : smoke_test_include_dirs,
        link_with           : malc_lib,
        link_args           : test_link_args,
        c_args              : cflags + test_cflags,
        dependencies        : threads
    ))

if cc.get_id() == 'gcc' or cc.get_id() == 'clang' # C++ compiler assumed if not
    configure_file(
        input : 'smoke_test/src/malc/smoke.c',
        output : 'smoke.cpp',
        configuration : configuration_data()
        )
    test(
        'malc',
        executable(
            'malc-smoke-cpp',
            [ meson.build_root() + '/smoke.cpp' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            cpp_args            : cflags + test_cflags,
            dependencies        : threads
        ))
endif

executable(
        'malc-example-hello-malc',
        [ 'example/src/hello-malc.c' ],
        include_directories : smoke_test_include_dirs,
        link_with           : malc_lib,
        link_args           : test_link_args,
        c_args              : cflags,
        dependencies        : threads
    )
executable(
        'malc-example-overwiew',
        [ 'example/src/overview.c' ],
        include_directories : smoke_test_include_dirs,
        link_with           : malc_lib,
        link_args           : test_link_args,
        c_args              : cflags,
        dependencies        : threads
    )
executable(
        'malc-example-thread-local-storage',
        [ 'example/src/thread_local_storage.c' ],
        include_directories : smoke_test_include_dirs,
        link_with           : malc_lib,
        link_args           : test_link_args,
        c_args              : cflags,
        dependencies        : threads
    )