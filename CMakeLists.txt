cmake_minimum_required(VERSION 3.16)

option(ENABLE_BUILD_CHECKS "Enable runtime scratch MMU boundary check" ON)

# Application Version
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 2)
set(APP_VERSION_PATCH 0)
set(APP_NAME "Edit")
set(target_name "edit")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)
find_package(ZVB_SDK REQUIRED)

zos_use(COREUTILS)
find_package(COREUTILS REQUIRED)

project(${target_name} C ASM)

add_executable(${target_name}
"src/main.c"
"src/edit.c"
"src/editor_state.c"
"src/document_store.c"
"src/editor_ui.c")

target_include_directories(${target_name} PRIVATE $ENV{ZVB_SDK_PATH}/include)
target_link_libraries(${target_name} PRIVATE core conio keyboard windows)

target_compile_options(${target_name} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:--nostdlib>
)

if(ENABLE_BUILD_CHECKS OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target_name} PRIVATE BUILD_VERIFY=1)
endif()

zos_add_outputs(${target_name})
# zos_add_outputs(${target_name} WITH_EXTENSION "")

add_custom_command(TARGET ${target_name}_bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/debug
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.cdb
        ${CMAKE_SOURCE_DIR}/debug/${target_name}.cdb
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.map
        ${CMAKE_SOURCE_DIR}/debug/${target_name}.map
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.ihx
        ${CMAKE_SOURCE_DIR}/debug/${target_name}.ihx
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}
    COMMENT "Moving debug files to debug directory and stripping .bin extension"
    VERBATIM
)
