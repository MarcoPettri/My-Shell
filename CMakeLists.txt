cmake_minimum_required(VERSION 3.25)
project(myshell VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

# Options
option(MYSHELL_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(MYSHELL_BUILD_TESTS "Build tests" ON)
set(MYSHELL_SANITIZERS "" CACHE STRING "Comma-separated list of sanitizers")

# System dependencies
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Try libedit via pkg-config
pkg_check_modules(LIBEDIT QUIET libedit)
if (NOT LIBEDIT_FOUND)
    find_path(LIBEDIT_INCLUDE_DIR histedit.h)
    find_library(LIBEDIT_LIBRARY edit)
    if (LIBEDIT_INCLUDE_DIR AND LIBEDIT_LIBRARY)
        set(LIBEDIT_FOUND TRUE)
        set(LIBEDIT_INCLUDE_DIRS ${LIBEDIT_INCLUDE_DIR})
        set(LIBEDIT_LIBRARIES ${LIBEDIT_LIBRARY})
    endif()
endif()

add_subdirectory(src)

if (MYSHELL_BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if (GTest_FOUND)
        add_subdirectory(tests)
    else()
        message(STATUS "GTest not found, checking pkg-config")
        pkg_check_modules(GTEST QUIET gtest_main)
        if (GTEST_FOUND)
            add_subdirectory(tests)
        else()
            message(WARNING "GTest not found. Tests disabled.")
        endif()
    endif()
endif()

# Formatting custom target
find_program(CLANG_FORMAT_EXE clang-format)
if (CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_CXX_SOURCE_FILES}
        COMMENT "Running clang-format on all source files"
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run -Werror ${ALL_CXX_SOURCE_FILES}
        COMMENT "Checking code format with clang-format"
    )
endif()
