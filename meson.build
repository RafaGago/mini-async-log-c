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
version_major = '1'
version_minor = '0'
version_rev   = '0'
version       = version_minor + '.' + version_major + '.' + version_rev

is_bare = get_option ('bare') ? 'true' : 'false'

base_library = subproject(
    'base_library',
    default_options : [
        'timestamp_64bit=true',
        'bare=' + is_bare
        ]
    )
base_lib     = base_library.get_variable ('base_lib')
nonblock_lib = base_library.get_variable ('nonblock_lib')
t_extras_lib = base_library.get_variable ('t_extras_lib')
if not get_option ('bare')
    libcmocka_path = base_library.get_variable ('libcmocka_path')
endif
build_helpers_dir = base_library.get_variable ('build_helpers_dir')

include_dirs = [
    include_directories ('include'),
    include_directories ('src'),
]
include_dirs += base_library.get_variable ('include_dirs')

if not get_option ('bare')
    test_include_dirs  = include_dirs
    test_include_dirs += [ include_directories ('test/src') ]
    test_include_dirs +=  base_library.get_variable ('test_include_dirs')

    smoke_test_include_dirs  = include_dirs
    smoke_test_include_dirs += [ include_directories ('smoke_test/src') ]
    smoke_test_include_dirs +=  base_library.get_variable ('test_include_dirs')
endif

pkg_mod = import ('pkgconfig')
install_subdir ('include/malc', install_dir : 'include')

cflags = base_library.get_variable ('cflags')
cflags += [ '-DMALC_LIBRARY_COMPILATION' ]
lib_cflags = []

if not get_option ('bare')
    test_cflags = []
    test_link_args = [ libcmocka_path ]
endif

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
    if not get_option ('bare')
        test_link_args += ['-lwinmm.lib'] # Untested
    endif
endif

threads = dependency ('threads')

cdata = configuration_data()

val = get_option ('compressed_builtins') ? '1' : '0'
cflags += [ '-DMALC_BUILTIN_COMPRESSION=' + val ]
cdata.set(
    'compressed_builtins_placeholder',
    '#define MALC_BUILTIN_COMPRESSION ' + val
     )

val = get_option ('compressed_ptrs') ? '1' : '0'
cflags += [ '-DMALC_PTR_COMPRESSION=' + val ]
cdata.set(
    'compressed_ptrs_placeholder',
    '#define MALC_PTR_COMPRESSION ' + val
     )

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
    link_with           : [ base_lib, nonblock_lib, t_extras_lib ],
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

if not get_option ('bare')
    test(
        'malc-test',
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
        'malc-smoke',
        executable(
            'malc-smoke',
            malc_smoke_test_srcs,
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            c_args              : cflags + test_cflags,
            dependencies        : threads
        ))

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
    st = executable(
            'malc-example-stress-test',
            [ 'example/src/stress-test.c' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            c_args              : cflags,
            dependencies        : threads
        )
    test ('malc-stress-test-tls', st, args : [ 'tls', '30', '1' ])
    test ('malc-stress-test-heap', st, args : [ 'heap', '30', '1' ])
    test ('malc-stress-test-queue', st, args : [ 'queue', '30', '1' ])
    test ('malc-stress-test-queue-cpu', st, args : [ 'queue-cpu', '30', '1' ])

    if cc.get_id() == 'gcc' or cc.get_id() == 'clang' # C++ compiler assumed if not
        configure_file(
            input : 'smoke_test/src/malc/smoke.c',
            output : 'smoke.cpp',
            copy: true
            )
        test(
            'malc-smoke-cpp',
            executable(
                'malc-smoke-cpp',
                [ meson.build_root() + '/smoke.cpp' ],
                include_directories : smoke_test_include_dirs,
                link_with           : malc_lib,
                link_args           : test_link_args,
                cpp_args            : cflags + test_cflags,
                dependencies        : threads
            ))
        test(
            'malc-cpp-wrapper',
            executable(
                'malc-example-cpp-wrapper',
                [ 'example/src/cpp-wrapper.cpp' ],
                include_directories : smoke_test_include_dirs,
                link_with           : malc_lib,
                link_args           : test_link_args,
                cpp_args            : cflags,
                dependencies        : threads
            ))
        configure_file(
            input : 'example/src/stress-test.c',
            output : 'stress-test.cpp',
            copy: true
            )
        executable(
            'malc-example-stress-test-cpp',
            [ meson.build_root() + '/stress-test.cpp' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            cpp_args            : cflags,
            dependencies        : threads
        )
    endif
endif
