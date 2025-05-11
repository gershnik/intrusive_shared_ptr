#cmake_policy(SET CMP0092 NEW)

if (WIN32)
set(CMAKE_SYSTEM_VERSION "10.0.19041.0" CACHE STRING "Windows SDK Version" FORCE)
endif()


include(FetchContent)

FetchContent_Declare(doctest
    URL  https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR downloaded/doctest
)

FetchContent_MakeAvailable(doctest)

if(ISPTR_ENABLE_PYTHON AND NOT Python3_Development_FOUND)
    find_package (Python3 COMPONENTS Development REQUIRED)
endif()


set (TEST_SUFFIXES
    17
    20
    23
)

set(CXX_STANDARD_17 17)
set(CXX_STANDARD_20 20)
set(CXX_STANDARD_23 23)

set(TEST_VARIANTS_17 "headers")
set(TEST_VARIANTS_20 "headers")
set(TEST_VARIANTS_23 "headers")

if (${ISPTR_ENABLE_MODULE})
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
        list(APPEND TEST_VARIANTS_20 "module")
        list(APPEND TEST_VARIANTS_23 "module")
    else()
        message(WARNING "CMake version ${CMAKE_VERSION} doesn't support modules, ignoring ISPTR_ENABLE_MODULE")
    endif()
endif()

set(TEST_COMMAND "")

foreach(TEST_SUFFIX ${TEST_SUFFIXES})

    foreach(TEST_VARIANT ${TEST_VARIANTS_${TEST_SUFFIX}})

        set(TEST_TARGET_NAME test-${TEST_SUFFIX}-${TEST_VARIANT})

        add_executable(${TEST_TARGET_NAME} EXCLUDE_FROM_ALL)

        set_target_properties(${TEST_TARGET_NAME} PROPERTIES
            CXX_STANDARD ${CXX_STANDARD_${TEST_SUFFIX}}
            CXX_STANDARD_REQUIRED OFF
            CXX_EXTENSIONS OFF
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON 
        )

        if("${TEST_VARIANT}" STREQUAL "module")
            set_target_properties(${TEST_TARGET_NAME} PROPERTIES
                CXX_SCAN_FOR_MODULES ON
            )
        endif()

        target_include_directories(${TEST_TARGET_NAME} PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/downloaded
        )

        if (ISPTR_ENABLE_PYTHON)
            target_link_libraries(${TEST_TARGET_NAME} PRIVATE
                ${Python3_LIBRARIES}
            )
            target_include_directories(${TEST_TARGET_NAME} SYSTEM PRIVATE
                ${Python3_INCLUDE_DIRS}
            )
        endif()

        target_link_libraries(${TEST_TARGET_NAME} PRIVATE
            "$<$<PLATFORM_ID:Darwin>:-framework CoreFoundation>"
            "$<$<NOT:$<STREQUAL:${TEST_VARIANT},module>>:isptr::isptr>"
        )

        target_compile_options(${TEST_TARGET_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>
            $<$<CXX_COMPILER_ID:Clang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
            $<$<CXX_COMPILER_ID:AppleClang>:-Wall;-Wextra;-pedantic;-Wno-self-assign-overloaded;-Wno-self-move>
            $<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-pedantic;-Wno-self-move>
        )

        target_compile_definitions(${TEST_TARGET_NAME} PRIVATE
            $<$<BOOL:${ISPTR_ENABLE_PYTHON}>:ISPTR_USE_PYTHON=1>
            $<$<STREQUAL:${TEST_VARIANT},module>:ISPTR_USE_MODULES=1>
            _FILE_OFFSET_BITS=64  # prevents weird issues with modules and clang on Ubuntu
        )

        if (${TEST_VARIANT} STREQUAL "module")
            isptr_add_module(${TEST_TARGET_NAME} PRIVATE)
        endif()

        target_sources(${TEST_TARGET_NAME} PRIVATE

            test_apple_cf_ptr.cpp
            test_atomic.cpp
            test_com_ptr.cpp
            test_python_ptr.cpp
            test_general.cpp
            test_main.cpp
            test_out_ptr.cpp
            test_ref_counted.cpp
            test_ref_counted_st.cpp
            test_weak_ref_counted.cpp
            test_weak_ref_counted_st.cpp
            test_abstract_ref_counted.cpp
            test_abstract_ref_counted_st.cpp
            test_delegating_traits.cpp

            mocks.h
        )

        list(APPEND TEST_COMMAND COMMAND)
        list(APPEND TEST_COMMAND echo Running: ${TEST_TARGET_NAME})
        list(APPEND TEST_COMMAND COMMAND)
        list(APPEND TEST_COMMAND ${TEST_TARGET_NAME} -ni -fc)

    endforeach()

endforeach()

add_custom_target(run-test
    ${TEST_COMMAND}
)
