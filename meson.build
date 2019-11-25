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
    default_options : [ 'bare=' + is_bare ]
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
malcpp_srcs = [
    'src/malc/cpp-wrapper/destinations.cpp',
    'src/malc/cpp-wrapper/wrapper.cpp',
    'src/malc/cpp-wrapper/malcpp-e-c-nd.cpp',
    'src/malc/cpp-wrapper/malcpp-ne-nc-nd.cpp',
    'src/malc/cpp-wrapper/malcpp-ne-nc-d.cpp',
    'src/malc/cpp-wrapper/malcpp-e-nc-d.cpp',
    'src/malc/cpp-wrapper/malcpp-e-nc-nd.cpp',
    'src/malc/cpp-wrapper/malcpp-e-c-d.cpp',
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
malc_test_cpp_srcs = [
    'test/src/malcpp/tests_main.cpp',
    'test/src/malcpp/log_entry_validator.cpp',
]
malc_smoke_test_srcs = [
    'smoke_test/src/malc/smoke.c',
]
malcpp_smoke_test_srcs = [
    'smoke_test/src/malcpp/smoke.cpp',
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
    malc_lib,
    version     : '0.0.1',
    name        : 'libmalc',
    filebase    : 'malc',
    description : 'mini-async-log-c'
    )

malcpp_lib = build_target(
    'malcpp',
    malcpp_srcs,
    target_type         : libtype,
    include_directories : include_dirs,
    link_with           : [ malc_lib ],
    c_args              : cflags + lib_cflags,
    install             : true
    )
pkg_mod.generate(
    malcpp_lib,
    version     : '0.0.1',
    name        : 'libmalcpp',
    filebase    : 'malcpp',
    description : 'mini-async-log-c c++ wrapper'
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
        'malcpp-test',
        executable(
            'malcpp-test',
            malc_test_cpp_srcs,
            include_directories : test_include_dirs,
            link_with           : [ malc_lib, malcpp_lib ],
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
    test(
        'malcpp-smoke',
        executable(
            'malcpp-smoke',
            malcpp_smoke_test_srcs,
            include_directories : smoke_test_include_dirs,
            link_with           : [ malc_lib, malcpp_lib ],
            link_args           : test_link_args,
            cpp_args            : cflags + test_cflags,
            dependencies        : threads
        ))

    executable(
            'malc-example-hello-malc',
            [ 'example/src/c11/hello-malc.c' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            c_args              : cflags,
            dependencies        : threads
        )
    executable(
            'malc-example-overwiew',
            [ 'example/src/c11/overview.c' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            c_args              : cflags,
            dependencies        : threads
        )
    executable(
            'malc-example-thread-local-storage',
            [ 'example/src/c11/thread_local_storage.c' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malc_lib,
            link_args           : test_link_args,
            c_args              : cflags,
            dependencies        : threads
        )
    st = executable(
            'malc-example-stress-test',
            [ 'example/src/c11/stress-test.c' ],
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

    executable(
        'malcpp-example-overview',
        [ 'example/src/c++11/overview.cpp' ],
        include_directories : smoke_test_include_dirs,
        link_with           : [ malcpp_lib, base_lib ],
        link_args           : test_link_args,
        cpp_args            : cflags,
        dependencies        : threads
        )
    executable(
        'malcpp-example-custom-destination',
        [ 'example/src/c++11/custom-destination.cpp' ],
        include_directories : smoke_test_include_dirs,
        link_with           : malcpp_lib,
        link_args           : test_link_args,
        cpp_args            : cflags,
        dependencies        : threads
        )
    executable(
        'malcpp-example-hello-malcpp',
        [ 'example/src/c++11/hello-malcpp.cpp' ],
        include_directories : smoke_test_include_dirs,
        link_with           : malcpp_lib,
        link_args           : test_link_args,
        cpp_args            : cflags,
        dependencies        : threads
        )

    stcpp = executable(
            'malcpp-example-stress-test',
            [ 'example/src/c++11/stress-test.cpp' ],
            include_directories : smoke_test_include_dirs,
            link_with           : malcpp_lib,
            link_args           : test_link_args,
            cpp_args            : cflags,
            dependencies        : threads
        )
    test ('malcpp-stress-test-tls', stcpp, args : [ 'tls', '30', '1' ])
    test ('malcpp-stress-test-heap', stcpp, args : [ 'heap', '30', '1' ])
    test ('malcpp-stress-test-queue', stcpp, args : [ 'queue', '30', '1' ])
    test(
        'malcpp-stress-test-queue-cpu', stcpp, args : [ 'queue-cpu', '30', '1' ]
        )
    # C++ specific compilation failure/pass test targets. these targets won't
    # build by default and are to be launched by a script invoking ninja, on
    # Linux this is done on "scripts/malcpp11-fmtstr-test.sh".

    validation_tests = {
        'cpp-fmtstr-excess-arguments-1': 'fail_excess_arguments_1.cpp',
        'cpp-fmtstr-excess-arguments-2': 'fail_excess_arguments_2.cpp',
        'cpp-fmtstr-excess-arguments-3': 'fail_excess_arguments_3.cpp',
        'cpp-fmtstr-excess-placecholders-1': 'fail_excess_placeholders_1.cpp',
        'cpp-fmtstr-excess-placecholders-2': 'fail_excess_placeholders_2.cpp',
        'cpp-fmtstr-excess-placecholders-3': 'fail_excess_placeholders_3.cpp',
        'cpp-fmtstr-unescaped-brace-1': 'fail_unescaped_brace_1.cpp',
        'cpp-fmtstr-unescaped-brace-2': 'fail_unescaped_brace_2.cpp',
        'cpp-fmtstr-same-argcount': 'pass_same_argcount.cpp',
        'cpp-fmtstr-bad-int-modifiers': 'fail_bad_int_modifiers.cpp',
        'cpp-fmtstr-bad-float-modifiers': 'fail_bad_float_modifiers.cpp',
        'cpp-fmtstr-missing-refdtor': 'fail_missing_refdtor.cpp',
        }
    foreach name, filename : validation_tests
        executable(
            name,
            ['test/src/malcpp/compile-time/' + filename ],
            link_with           : [ malcpp_lib, base_lib ],
            include_directories : test_include_dirs,
            cpp_args            : cflags,
            dependencies        : threads,
            build_by_default    : false
            )
    endforeach
endif
