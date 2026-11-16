# Elwro 800 Junior emulator

cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

if(WIN32)
    project(elwro LANGUAGES C RC)
else()
    project(elwro LANGUAGES C)
endif()

option(DEBUG_FDC "Debug FDC" OFF)

# generatory jednokonfiguracyjne (Ninja, Makefiles): domyslnie Release, inaczej kod bylby bez optymalizacji
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# LTO pozwala wstawic inline wywolania pamieci/IO do rdzenia Z80
include(CheckIPOSupported)
check_ipo_supported(RESULT ELWRO_IPO OUTPUT ELWRO_IPO_MSG LANGUAGES C)

include(FetchContent)

set(CACHE{BUILD_SHARED_LIBS} TYPE INTERNAL VALUE OFF)

find_package(OpenGL REQUIRED)

FetchContent_Declare(
    libspectrum
    GIT_REPOSITORY https://github.com/apwojcik/libspectrum.git
    GIT_TAG develop
    GIT_SHALLOW TRUE
    GIT_PROGRESS FALSE
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(libspectrum)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    GIT_PROGRESS FALSE
    SYSTEM
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(SDL3)

add_executable(elwro WIN32 MACOSX_BUNDLE
        src/e800j.c
        src/e800j.h
        src/e800j_cfg.c
        src/e800j_cfg.h
        src/e800j_cmdline.c
        src/e800j_cmdline.h
        src/e800j_compat.c
        src/e800j_compat.h
        src/e800j_config.h
        src/e800j_disk.c
        src/e800j_disk.h
        src/e800j_fdc.c
        src/e800j_fdc.h
        src/e800j_file.c
        src/e800j_file.h
        src/e800j_io.c
        src/e800j_io.h
        src/e800j_kbrd.c
        src/e800j_kbrd.h
        src/e800j_mem.c
        src/e800j_mem.h
        src/e800j_msg.c
        src/e800j_msg.h
        src/e800j_prom.h
        src/e800j_sound.c
        src/e800j_sound.h
        src/i8272_fdc.c
        src/i8272_fdc.h
        src/virt_fdd.c
        src/virt_fdd.h
        src/z80_core.c
        src/z80_core.h
        src/z80_cpu.h
        src/z80_debug.c
        src/z80_debug.h
        src/z80_dtbl.h
        src/zx_scrn.c
        src/zx_scrn.h)

if(APPLE)
    # elwro.app: start bez Terminala; konfiguracja i ROM-y trafiaja do Contents/Resources
    file(GLOB ELWRO_ROMS "${CMAKE_SOURCE_DIR}/rom/*")
    target_sources(elwro PRIVATE "${CMAKE_SOURCE_DIR}/e800j.cfg" ${ELWRO_ROMS})
    set_source_files_properties("${CMAKE_SOURCE_DIR}/e800j.cfg" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${ELWRO_ROMS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/rom)
    target_sources(elwro PRIVATE "${CMAKE_SOURCE_DIR}/res/elwro.icns")
    set_source_files_properties("${CMAKE_SOURCE_DIR}/res/elwro.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(elwro PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Elwro 800 Junior"
        MACOSX_BUNDLE_ICON_FILE "elwro.icns"
        MACOSX_BUNDLE_GUI_IDENTIFIER "pl.elwro.e800j"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.3"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.3")
endif()

if(ELWRO_IPO)
    set_property(TARGET elwro PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

if(DEBUG_FDC)
    target_compile_definitions(elwro PRIVATE DEBUG_FDC)
endif()

target_link_libraries(elwro PRIVATE SDL3::SDL3-static libspectrum::spectrum OpenGL::GL OpenGL::GLU)

if(WIN32)
    # ikona aplikacji (res/elwro.ico) wkompilowana w exe
    target_sources(elwro PRIVATE res/elwro.rc)
    target_compile_definitions(elwro PRIVATE LIBSPECTRUM_EXPORTS _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

if(MSVC)
    target_compile_options(elwro PRIVATE /W4)
else()
    target_compile_options(elwro PRIVATE -Wall -Wextra -pedantic)
    target_link_libraries(elwro PRIVATE m)
endif()
