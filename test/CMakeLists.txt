cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0092 NEW)

if (WIN32)
set(CMAKE_SYSTEM_VERSION "10.0.19041.0" CACHE STRING "Windows SDK Version" FORCE)
endif()


project(test)

find_package (Python3 COMPONENTS Interpreter Development)

if(${Python3_Development_FOUND})
    include_directories(
    SYSTEM
        ${Python3_INCLUDE_DIRS}
    )

    link_libraries(
        ${Python3_LIBRARIES}
    )
endif()

add_subdirectory(..  lib)

set (TEST_SUFFIXES
    17
    20
)

set(CXX_STANDARD_17 17)
set(CXX_STANDARD_20 20)

set(TEST_COMMAND "")

foreach(TEST_SUFFIX ${TEST_SUFFIXES})

    add_executable(test-${TEST_SUFFIX})

    set_target_properties(test-${TEST_SUFFIX} PROPERTIES
        CXX_STANDARD ${CXX_STANDARD_${TEST_SUFFIX}}
        CXX_STANDARD_REQUIRED OFF
        CXX_EXTENSIONS OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_link_libraries(test-${TEST_SUFFIX} PRIVATE
        intrusive-shared-ptr
    )

    target_compile_options(test-${TEST_SUFFIX} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>
        $<$<CXX_COMPILER_ID:Clang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
        $<$<CXX_COMPILER_ID:AppleClang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
        $<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-pedantic>
    )

    target_link_libraries(test-${TEST_SUFFIX} PRIVATE
        "$<$<PLATFORM_ID:Darwin>:-framework CoreFoundation>"
    )

    target_compile_definitions(test-${TEST_SUFFIX} PRIVATE
        $<$<BOOL:${Python3_Development_FOUND}>:ISPTR_USE_PYTHON=1>
    )

    target_sources(test-${TEST_SUFFIX} PRIVATE

        test_apple_cf_ptr.cpp
        test_atomic.cpp
        test_com_ptr.cpp
        test_python_ptr.cpp
        test_general.cpp
        test_main.cpp
        test_ref_counted.cpp
        test_weak_ref_counted.cpp
        test_abstract_ref_counted.cpp
        test_delegating_traits.cpp

        mocks.h
        catch.hpp
    )

    list(APPEND TEST_COMMAND COMMAND)
    list(APPEND TEST_COMMAND test-${TEST_SUFFIX})

endforeach()

add_custom_target(run-test 
    ${TEST_COMMAND}
)
