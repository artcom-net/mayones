cmake_minimum_required(VERSION 4.1.2)

option(BUILD_TESTING "Build tests" OFF)
option(ENABLE_LTO "Enable IPO/LTO" OFF)
option(ENABLE_WERROR "Treat warnings as error" OFF)

if(BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

set(MAYONES_BASE_VERSION "0.1.0")
set(MAYONES_GIT_VERSION "0.1.0-dev")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(GIT_DESCRIBE)
        set(MAYONES_GIT_VERSION "${GIT_DESCRIBE}")
        if(GIT_DESCRIBE MATCHES "^v?([0-9]+\\.[0-9]+\\.[0-9]+)")
            set(MAYONES_BASE_VERSION "${CMAKE_MATCH_1}")
        endif()
    endif()
endif()

project (
    "mayones"
    DESCRIPTION "NES emulator"
    LANGUAGES CXX
    VERSION ${MAYONES_BASE_VERSION}
)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT MAYONES_LTO_SUPPORTED OUTPUT MAYONES_LTO_ERROR)
    if(NOT MAYONES_LTO_SUPPORTED)
        message(WARNING "IPO/LTO not supported: ${MAYONES_LTO_ERROR}")
    endif()
endif()

set(MAYONES_EXE "${PROJECT_NAME}")
set(MAYONES_APP "${PROJECT_NAME}_app")
set(MAYONES_FLAGS "${PROJECT_NAME}_flags")

add_library(${MAYONES_FLAGS} INTERFACE)
target_compile_features(${MAYONES_FLAGS} INTERFACE cxx_std_23)
target_compile_options(${MAYONES_FLAGS} INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

function(mayones_setup_target target)
    set(options DISABLE_LTO DISABLE_WERROR)
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target '${target}' doesn't exist. Cannot set up properties.")
    endif()

    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS NO)

    if(NOT ARG_DISABLE_WERROR AND ENABLE_WERROR)
        set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
    endif()

    if(NOT ARG_DISABLE_LTO AND ENABLE_LTO AND MAYONES_LTO_SUPPORTED)
        set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()

add_subdirectory(src/app)
add_subdirectory(src)

if(BUILD_TESTING)
    if(MSVC)
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    enable_testing()
    add_subdirectory(tests)
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(compile_commands_filename "compile_commands.json")
    message(STATUS "Linking ${compile_commands_filename} to source root...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/${compile_commands_filename}"
            "${CMAKE_SOURCE_DIR}/${compile_commands_filename}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
