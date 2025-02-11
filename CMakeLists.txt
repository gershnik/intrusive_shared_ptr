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


option(ISPTR_PROVIDE_MODULE "whether C++ module target should be provided" OFF)
option(ISPTR_ENABLE_PYTHON "whether to enable Python support in C++ module" OFF)


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

if(ISPTR_PROVIDE_MODULE)

    if(ISPTR_ENABLE_PYTHON AND NOT Python3_Development_FOUND)
        find_package (Python3 COMPONENTS Development REQUIRED)
    endif()

    add_library(${LIBNAME}m_helper INTERFACE)
    add_library(${LIBNAME}m::helper ALIAS ${LIBNAME}m_helper)

    if (ISPTR_ENABLE_PYTHON)

        target_include_directories(${LIBNAME}m_helper SYSTEM
        INTERFACE
            ${Python3_INCLUDE_DIRS}
        )

        target_link_libraries(${LIBNAME}m_helper 
        INTERFACE
            ${Python3_LIBRARIES}
        )

        target_compile_definitions(${LIBNAME}m_helper 
        INTERFACE
            ISPTR_ENABLE_PYTHON=1
        )

    endif()

    add_library(${LIBNAME}m OBJECT)

    target_compile_features(${LIBNAME}m
    PUBLIC
        cxx_std_20
    )

    set_target_properties(${LIBNAME}m PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON 
    )

    target_link_libraries(${LIBNAME}m
    PRIVATE
        $<BUILD_INTERFACE:${LIBNAME}m::helper>
    )

    target_sources(${LIBNAME}m
    PUBLIC 
        FILE_SET CXX_MODULES BASE_DIRS ${SRCDIR}/modules FILES
            ${SRCDIR}/modules/isptr.cppm
    )

    add_library(${LIBNAME}m::${LIBNAME}m ALIAS ${LIBNAME}m)

endif()

if (PROJECT_IS_TOP_LEVEL)

    include(cmake/install.cmake)

    add_subdirectory(test)

endif()