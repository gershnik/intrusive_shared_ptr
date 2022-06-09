#
# Copyright 2004 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
#

cmake_minimum_required(VERSION 3.20)


set(SRCDIR ${CMAKE_CURRENT_LIST_DIR})
set(LIBNAME intrusive-shared-ptr)

add_library(${LIBNAME} INTERFACE)

target_include_directories(${LIBNAME}
INTERFACE
    ${SRCDIR}/inc
)

target_sources(${LIBNAME} 
PRIVATE 
    ${SRCDIR}/inc/intrusive_shared_ptr/intrusive_shared_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/apple_cf_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/com_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/refcnt_ptr.h
    ${SRCDIR}/inc/intrusive_shared_ptr/ref_counted.h
)

