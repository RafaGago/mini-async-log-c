# See https://github.com/anholt/libepoxy/blob/master/meson.build
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

base_library      = subproject ('base_library')
base_lib          = base_library.get_variable ('base_lib')
nonblock_lib      = base_library.get_variable ('nonblock_lib')
libcmocka_path    = base_library.get_variable ('libcmocka_path')
build_helpers_dir = base_library.get_variable ('build_helpers_dir')

include_dirs = [
#    include_directories ('include'),
    include_directories ('src'),
]
include_dirs += base_library.get_variable ('include_dirs')

test_include_dirs  = include_dirs
#test_include_dirs += [ include_directories ('test/src') ]
test_include_dirs +=  base_library.get_variable ('test_include_dirs')

pkg_mod = import ('pkgconfig')
#install_subdir ('include/malc', install_dir : 'include')

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

#- malc ------------------------------------------------------------------------

malc_srcs = [
]
malc_test_srcs = [
    'src/malc/proof_of_concept_main.c',
]

#cflags += [ '-E' ] #erase
executable(
    'malc-proof-of-concept',
    malc_test_srcs,
    include_directories : include_dirs,
    link_with           : [ base_lib, ],
    c_args              : cflags,
    dependencies        : threads
    )

#malc_lib = build_target(
#    'malc',
#    malc_srcs,
#    target_type         : libtype,
#    include_directories : include_dirs,
#    link_with           : [ base_lib, nonblock_lib ],
#    c_args              : cflags + lib_cflags,
#    install             : true
#    )
#pkg_mod.generate(
#    libraries   : malc_lib,
#    version     : '0.0.1',
#    name        : 'libmalc',
#    filebase    : 'malc',
#    description : 'simulator task scheduler'
#    )
#test(
#    'malc',
#    executable(
#        'malc-test',
#        malc_test_srcs,
#        include_directories : test_include_dirs,
#        link_with           : malc_lib,
#        link_args           : test_link_args,
#        dependencies        : threads
#    ))
