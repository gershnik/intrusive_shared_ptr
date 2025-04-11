#
# Copyright 2004 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
#

cmake_minimum_required(VERSION 3.25)

file(READ VERSION ISPTR_VERSION)
if (NOT ISPTR_VERSION)
    message(FATAL_ERROR "Cannot determine library version (VERSION file not found)")
endif()
string(STRIP ${ISPTR_VERSION} ISPTR_VERSION)

project(isptr VERSION ${ISPTR_VERSION} LANGUAGES CXX)

set(SRCDIR ${CMAKE_CURRENT_LIST_DIR})
set(LIBNAME isptr)

add_library(${LIBNAME} INTERFACE)

target_include_directories(${LIBNAME}
INTERFACE
    $<BUILD_INTERFACE:${SRCDIR}/inc>
    $<INSTALL_INTERFACE:include>  # <prefix>/include
)

set(PUBLIC_HEADERS 
    ${SRCDIR}/inc/intrusive_shared_ptr/intrusive_shared_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/common.h
    ${SRCDIR}/inc/intrusive_shared_ptr/apple_cf_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/com_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/python_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/refcnt_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/ref_counted.h
)

target_sources(${LIBNAME} 
INTERFACE 
    FILE_SET HEADERS BASE_DIRS ${SRCDIR}/inc FILES
        ${PUBLIC_HEADERS}
PRIVATE 
    ${PUBLIC_HEADERS}
)

add_library(intrusive-shared-ptr ALIAS ${LIBNAME})
add_library(${LIBNAME}::${LIBNAME} ALIAS ${LIBNAME})

set(ISPTR_MODULES_BASE_DIRS ${SRCDIR}/modules)
set(ISPTR_MODULES_FILES ${SRCDIR}/modules/isptr.cppm)

function(isptr_add_module target_name visibility)
    target_sources(${target_name} ${visibility}
        FILE_SET isptr_module TYPE CXX_MODULES BASE_DIRS ${ISPTR_MODULES_BASE_DIRS} FILES ${ISPTR_MODULES_FILES}
    )
endfunction()


if (PROJECT_IS_TOP_LEVEL)

    include(cmake/install.cmake)

    add_subdirectory(test)

endif()