# Elwro 800 Junior emulator

cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

project(elwro LANGUAGES C)

option(USE_SDL "Use SDL" ON)
option(DEBUG_FDC "Debug FDC" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libspectrum REQUIRED libspectrum IMPORTED_TARGET)
find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)

add_executable(elwro
        src/e800j.c
        src/e800j.h
        src/e800j_compat.c
        src/e800j_compat.h
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

if(DEBUG_FDC)
    target_compile_definitions(elwro PRIVATE DEBUG_FDC)
endif()

target_link_libraries(elwro PRIVATE SDL3::SDL3 PkgConfig::libspectrum OpenGL::GL OpenGL::GLU)
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
