#
# Copyright 2004 Eugene Gershnik
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS isptr EXPORT isptr FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT isptr NAMESPACE isptr:: FILE isptr-exports.cmake DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/isptr)

install(FILES ${SRCDIR}/modules/isptr.cppm DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/isptr)

# Clean the old CMAKE_INSTALL_LIBDIR location where things were previously put
# on older versions
install(CODE "file(REMOVE_RECURSE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/isptr\")")


configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/isptr-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/isptr-config.cmake
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/isptr
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/isptr-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/isptr-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/isptr-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/isptr
)

file(RELATIVE_PATH FROM_PCFILEDIR_TO_PREFIX ${CMAKE_INSTALL_FULL_DATAROOTDIR}/isptr ${CMAKE_INSTALL_PREFIX})
string(REGEX REPLACE "/+$" "" FROM_PCFILEDIR_TO_PREFIX "${FROM_PCFILEDIR_TO_PREFIX}") 

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/isptr.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/isptr.pc
    @ONLY
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/isptr.pc
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)