# Elwro 800 Junior emulator

cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

project(elwro LANGUAGES C)

option(DEBUG_FDC "Debug FDC" OFF)

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
        #src/e800j_string.h
        #src/e800j_string.c
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

target_link_libraries(elwro PRIVATE SDL3::SDL3-static libspectrum::spectrum OpenGL::GL OpenGL::GLU)

if(WIN32)
    target_compile_definitions(elwro PRIVATE LIBSPECTRUM_EXPORTS _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

if(MSVC)
    target_compile_options(elwro PRIVATE /W4)
else()
    target_compile_options(elwro PRIVATE -Wall -Wextra -pedantic)
    target_link_libraries(elwro PRIVATE m)
endif()
