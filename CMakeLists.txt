cmake_minimum_required(VERSION 3.21)

# /Z7 embeds debug info in .obj files (no mspdbsrv / VC143.PDB), avoiding CL crash on Release builds.
if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDRPatternReader VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Set GEODE_SDK to your Geode SDK path.")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

# Avoid MSVC CL.exe crash (MSB6006 / CloseTypeServerPDB) on Release builds.
if(MSVC)
    # GEODE_DISABLE_PRECOMPILED_HEADERS only affects Geode SDK targets; the mod target
    # still got PCH from CMake/Geode unless we disable it here (PCH + optimized codegen
    # has triggered CL access violations on some VS 17.14 / Win SDK combos).
    set_target_properties(${PROJECT_NAME} PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
        MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Release>:None>$<$<NOT:$<CONFIG:Release>>:Embedded>"
    )
    target_compile_options(${PROJECT_NAME} PRIVATE /FS /MP1 /GL-)
    # If CL still crashes during Release codegen (MSB6006 / 0xC0000005), /Ob1 limits
    # cross-TU inlining and often avoids the buggy path; remove if builds are too slow.
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Ob1>)
    target_link_options(${PROJECT_NAME} PRIVATE
        /LTCG:OFF
        /INCREMENTAL:NO
        $<$<CONFIG:Release>:/DEBUG:NONE>
    )
    if(DEFINED ENV{GEODE_SDK})
        set(_geode_entry "$ENV{GEODE_SDK}/entry.cpp")
        if(EXISTS "${_geode_entry}")
            set_source_files_properties("${_geode_entry}" PROPERTIES
                COMPILE_OPTIONS "$<$<CONFIG:Release>:/MP1;/GL->"
            )
        endif()
    endif()
endif()
