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
version       = version_major + '.' + version_minor + '.' + version_rev

is_bare = get_option ('bare') ? 'true' : 'false'

base_library = subproject(
    'base_library',
    default_options : [ 'bare=' + is_bare ]
    )
base_lib     = base_library.get_variable ('base_lib')
nonblock_lib = base_library.get_variable ('nonblock_lib')
t_extras_lib = base_library.get_variable ('t_extras_lib')

include_dirs = [
    include_directories ('include'),
    include_directories ('src'),
]
include_dirs += base_library.get_variable ('include_dirs')

if not get_option ('bare')
    test_include_dirs  = include_dirs
    test_include_dirs += [ include_directories ('test/src') ]
    test_include_dirs +=  base_library.get_variable ('test_include_dirs')
endif

pkg_mod = import ('pkgconfig')
install_subdir ('include/malc', install_dir : 'include')
install_subdir ('include/malcpp', install_dir : 'include')

cflags = base_library.get_variable ('cflags')
cflags += [ '-DMALC_LIBRARY_COMPILATION' ]
lib_cflags = []

if not get_option ('bare')
    test_cflags = []
endif

if get_option ('sharedlib')
    libtype = 'shared_library'
else
    libtype = 'static_library'
endif

cc = meson.get_compiler ('c')
host_system = host_machine.system()
if cc.get_id() == 'gcc' or cc.get_id() == 'clang' or cc.get_id() == 'intel'
    cflags += ['-fvisibility=hidden', '-fno-stack-protector']
    if get_option ('pic_statlibs') and libtype == 'static_library'
        lib_cflags += ['-fPIC']
    endif
endif

if not get_option ('bare')
    cmocka = subproject('cmocka')
    cmocka_dep = cmocka.get_variable ('dep_cmocka')
endif

cpp_only = false
platform_deps = []
if host_system == 'windows'
    platform_deps += cc.find_library ('winmm')

    if cc.get_id() == 'msvc' or \
       cc.get_id() == 'intel-cl' or \
       cc.get_id() == 'clang-cl'

        cpp_only = true
        cflags += ['/TP'] # compile C as C++
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

val = get_option ('ptr_bytes_cut_count')
cflags += [ '-DMALC_PTR_MSB_BYTES_CUT_COUNT=' + val.to_string() ]
cdata.set(
    'compressed_ptrs_placeholder',
    '#define MALC_PTR_MSB_BYTES_CUT_COUNT ' + val.to_string()
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
    'src/malcpp/destinations.cpp',
    'src/malcpp/wrapper.cpp',
    'src/malcpp/c++_std_types.cpp',
    'src/malcpp/custom_type.cpp',
    'src/malcpp/malcpp-e-c-nd.cpp',
    'src/malcpp/malcpp-ne-nc-nd.cpp',
    'src/malcpp/malcpp-ne-nc-d.cpp',
    'src/malcpp/malcpp-e-nc-d.cpp',
    'src/malcpp/malcpp-e-nc-nd.cpp',
    'src/malcpp/malcpp-e-c-d.cpp',
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
    'test/src/malcpp/log_entry_validator_c++_types.cpp',
]
malc_smoke_test_srcs = [
    'test/src/malc-smoke/smoke.c',
]
malcpp_smoke_test_srcs = [
    'test/src/malcpp-smoke/smoke.cpp',
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
            c_args              : cflags + test_cflags,
            dependencies        : [threads, cmocka_dep]
        ))
    test(
        'malcpp-test',
        executable(
            'malcpp-test',
            malc_test_cpp_srcs,
            include_directories : test_include_dirs,
            link_with           : [ malc_lib, malcpp_lib ],
            c_args              : cflags + test_cflags,
            dependencies        : [threads, cmocka_dep]
        ))
    if not cpp_only
        test(
            'malc-smoke',
            executable(
                'malc-smoke',
                malc_smoke_test_srcs,
                include_directories : test_include_dirs,
                link_with           : malc_lib,
                c_args              : cflags + test_cflags,
                dependencies        : [threads, cmocka_dep]
            ))
    endif
    test(
        'malcpp-smoke',
        executable(
            'malcpp-smoke',
            malcpp_smoke_test_srcs,
            include_directories : test_include_dirs,
            link_with           : [ malc_lib, malcpp_lib ],
            cpp_args            : cflags + test_cflags,
            dependencies        : [threads, cmocka_dep]
        ))

    if not cpp_only
        executable(
                'malc-example-hello-malc',
                [ 'example/src/malc/hello-malc.c' ],
                include_directories : test_include_dirs,
                link_with           : malc_lib,
                c_args              : cflags,
                dependencies        : threads
            )
        executable(
                'malc-example-overview',
                [ 'example/src/malc/overview.c' ],
                include_directories : test_include_dirs,
                link_with           : malc_lib,
                c_args              : cflags,
                dependencies        : threads
            )
        executable(
                'malc-example-thread-local-storage',
                [ 'example/src/malc/thread_local_storage.c' ],
                include_directories : test_include_dirs,
                link_with           : malc_lib,
                c_args              : cflags,
                dependencies        : threads
            )
        st = executable(
                'malc-example-stress-test',
                [ 'example/src/malc/stress-test.c' ],
                include_directories : test_include_dirs,
                link_with           : malc_lib,
                c_args              : cflags,
                dependencies        : threads
            )
        test ('malc-stress-test-tls', st, args : [ 'tls', '30', '1' ])
        test ('malc-stress-test-heap', st, args : [ 'heap', '30', '1' ])
        test ('malc-stress-test-queue', st, args : [ 'queue', '30', '1' ])
        test(
            'malc-stress-test-queue-cpu', st, args : [ 'queue-cpu', '30', '1' ]
            )
    endif
    executable(
        'malcpp-example-overview',
        [ 'example/src/malcpp/overview.cpp' ],
        include_directories : test_include_dirs,
        link_with           : [ malcpp_lib, base_lib ],
        cpp_args            : cflags,
        dependencies        : threads
        )
    executable(
        'malcpp-example-custom-destination',
        [ 'example/src/malcpp/custom-destination.cpp' ],
        include_directories : test_include_dirs,
        link_with           : malcpp_lib,
        cpp_args            : cflags,
        dependencies        : threads
        )
    executable(
        'malcpp-example-hello-malcpp',
        [ 'example/src/malcpp/hello-malcpp.cpp' ],
        include_directories : test_include_dirs,
        link_with           : malcpp_lib,
        cpp_args            : cflags,
        dependencies        : threads
        )
    executable(
        'malcpp-example-custom-type',
        [ 'example/src/malcpp/custom-type.cpp' ],
        include_directories : test_include_dirs,
        link_with           : malcpp_lib,
        cpp_args            : cflags,
        dependencies        : threads
        )
    stcpp = executable(
            'malcpp-example-stress-test',
            [ 'example/src/malcpp/stress-test.cpp' ],
            include_directories : test_include_dirs,
            link_with           : malcpp_lib,
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
