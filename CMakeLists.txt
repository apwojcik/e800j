# Elwro 800 Junior emulator

cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

project(elwro LANGUAGES C)

option(USE_SDL "Use SDL" ON)
option(DEBUG_FDC "Debug FDC" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libspectrum REQUIRED libspectrum IMPORTED_TARGET)
find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(elwro
        e800j.c
        e800j.h
        e800j_fdc.c
        e800j_fdc.h
        e800j_file.c
        e800j_file.h
        e800j_io.c
        e800j_io.h
        e800j_kbrd.c
        e800j_kbrd.h
        e800j_mem.c
        e800j_mem.h
        e800j_msg.c
        e800j_msg.h
        e800j_prom.h
        e800j_sound.c
        e800j_sound.h
        i8272_fdc.c
        i8272_fdc.h
        virt_fdd.c
        virt_fdd.h
        z80_core.c
        z80_core.h
        z80_cpu.h
        z80_debug.c
        z80_debug.h
        z80_dtbl.h
        zx_scrn.c
        zx_scrn.h)

if(DEBUG_FDC)
    target_compile_definitions(elwro PRIVATE DEBUG_FDC)
endif()

target_link_libraries(elwro PRIVATE SDL::SDL PkgConfig::libspectrum OpenGL::GL OpenGL::GLU)
target_compile_definitions(elwro PRIVATE _USE_SDL)

if(WIN32)
    target_compile_definitions(elwro PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

if(MSVC)
    target_compile_options(elwro PRIVATE /W4)
else()
    target_compile_options(elwro PRIVATE -Wall -Wextra -pedantic)
    target_link_libraries(elwro PRIVATE m)
endif()
