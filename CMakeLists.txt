cmake_minimum_required(VERSION 3.15.0)

if ($ENV{EXPORT_COMPILE_COMMANDS})
    # ccls needs compile_commands.json to work
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId} C)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE PROJECT_SOURCE_FILES "src/*.c")
# note: if you want to change main target name (aka executable name)
# make sure you also change it in run.sh and run.bat
# set(MIAN_TARGET ${PROJECT_NAME})
set(MAIN_TARGET app)
add_executable(${MAIN_TARGET} ${PROJECT_SOURCE_FILES})

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${MAIN_TARGET} PRIVATE -Wall -Wextra -Wpedantic -O3 $<$<CONFIG:Debug>:-pg>)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${MAIN_TARGET} PRIVATE -Wall -Wextra -Wpedantic -O3 $<$<CONFIG:Debug>:-pg>)
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	# message(FATAL_ERROR "NO GOD PLEASE NO! use clang or minGW if you are on windows")
	target_compile_options(${MAIN_TARGET} PRIVATE /J /W4 /Oi $<$<CONFIG:Release>:/O2>)
endif()
if (WIN32)
	target_compile_definitions(${MAIN_TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if ($ENV{COLOR_DIAGNOSTICS})
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${MAIN_TARGET} PRIVATE -fcolor-diagnostics)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${MAIN_TARGET} PRIVATE -fdiagnostics-color=always)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        # undocumented; untested
        target_compile_options(${MAIN_TARGET} PRIVATE /diagnostics:color)
    endif()
endif()

# target_link_libraries(${MAIN_TARGET} PRIVATE)
target_include_directories(${MAIN_TARGET} PRIVATE "include/")

