#
# Copyright 2004 Eugene Gershnik
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
#

# Meson port of the CMake build 
#
# What this provides, mirroring CMakeLists.txt / cmake/install.cmake:
#
#   1. A header-only dependency that can be installed locally
#      (headers + a pkg-config file), so external builds can find it.
#   2. A dependency object other Meson builds can consume, both as a
#      subproject (via `dependency('isptr')`) and once installed
#      (via pkg-config).
#   3. A way for a dependent project to compile the C++ module
#      `modules/isptr.cppm` into one of its targets -- the Meson
#      equivalent of CMake's `isptr_add_module(target, visibility)`.

project(
    'isptr',
    'cpp',
    version : files('VERSION'),
    license : 'MIT',
    meson_version : '>=0.63.0',
    # Meson defaults to absolute paths in pkgconfig, yuck!
    default_options : ['pkgconfig.relocatable=true'],
)

#
# ---- Header-only library -------------------------------------------------
#

isptr_inc = include_directories('inc')

isptr_module_file = files('modules' / 'isptr.cppm')

# The header-only dependency. We attach an informational variable so a
# *subproject* consumer can locate the module source the same way an
# *installed* consumer locates it through pkg-config (see get_variable usage
# in the README snippet below).
isptr_dep = declare_dependency(
    include_directories : isptr_inc,
    version : meson.project_version(),
    variables : {
        'module' : meson.current_source_dir() / 'modules' / 'isptr.cppm',
    },
)

# Make `dependency('isptr')` resolve to the above when this project is used
# as a Meson subproject. 
meson.override_dependency('isptr', isptr_dep)

#
# ---- C++ module ----------------------------------------------------------
#
# A subproject consumer uses it as:
#
#       isptr_module_dep = dependency('isptr-module')
#       executable('app', 'main.cpp', dependencies : isptr_module_dep)
#
isptr_module_dep = declare_dependency(
    sources : isptr_module_file,
    version : meson.project_version(),
)

meson.override_dependency('isptr-module', isptr_module_dep)

#
# ---- Installation --------------------------------------------------------
#
#
if not meson.is_subproject()

    # Headers: install the whole `intrusive_shared_ptr` directory, preserving
    # the subdir so `<intrusive_shared_ptr/...>` keeps working.
    install_subdir(
        'inc' / 'intrusive_shared_ptr',
        install_dir : get_option('includedir'),
    )

    # The C++ module source. Installed under <datadir>/isptr (share/isptr) to
    # match the CMake build. It is arch-independent, so it belongs in share, not
    # libdir. Consumers locate it through the `module` pkg-config variable below.
    module_install_dir = get_option('datadir') / 'isptr'
    install_data(
        isptr_module_file,
        install_dir : module_install_dir,
    )

    # pkg-config file (replaces cmake/isptr.pc.in). A `module` variable lets
    # installed consumers locate the .cppm to add it to their own target.
    pkg = import('pkgconfig')
    pkg.generate(
        name : 'isptr',
        description : 'Yet another implementation of an intrusive reference counting for C++',
        version : meson.project_version(),
        # Header search path. Headers live at <includedir>/intrusive_shared_ptr,
        # and code includes them as <intrusive_shared_ptr/...>, so the search
        # root is <includedir> itself (the default subdir '.').
        subdirs : ['.'],
        variables : [
            'module=' + ('${prefix}' / get_option('datadir') / 'isptr' / 'isptr.cppm'),
        ],
        install_dir : get_option('datadir') / 'pkgconfig',
    )

endif

