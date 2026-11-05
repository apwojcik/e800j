# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

An emulator for the **Elwro 800 Junior**, a Polish 1980s school computer that is ~100% ZX Spectrum
48K compatible and additionally supports its own CP/J (CP/M clone) mode. Original author: Krzysztof
Komarnicki (2006); current work modernizes the codebase (build system, C cleanup; SDL1 to SDL3 migration is done).

Source comments and docs (`doc/Readme`, `doc/ToDo`) are written in Polish; preserve that language in
existing comments rather than translating them.

## Build

CMake (>= 4.2) + Ninja. `libspectrum` (fork `apwojcik/libspectrum`, branch `develop`) and SDL3
(`libsdl-org/SDL`, branch `main`) are fetched via `FetchContent` at configure time and linked
statically (`SDL3::SDL3-static`, `libspectrum::spectrum`), so the first configure needs network access.
OpenGL/GLU are the only system dependencies (`find_package(OpenGL)`). All C sources and headers live in `src/` (the architecture section below refers to files by bare name, e.g. `e800j.c` = `src/e800j.c`).

```sh
cmake -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug

cmake -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

The project is also opened directly in CLion/Visual Studio, which manage the `build/Debug` and
`build/Release` directories as CMake profiles — prefer reusing those directories over inventing new
build folders.

Key CMake options (`CMakeLists.txt`):
- `USE_SDL` (default ON)
- `DEBUG_FDC` (default OFF) — enables verbose floppy controller debug output via `DEBUG_FDC` define

Warnings are enabled (`-Wall -Wextra -pedantic` on GCC/Clang, `/W4` on MSVC) and the code is kept
warning-free on both toolchains; Windows/MSVC builds are supported, so keep changes portable.

There is no test suite in this repository.

## Running

The built `elwro` executable reads `e800j.cfg` from the current working directory for hardware/profile
configuration (BIOS ROM set, CPU timing, screen palette, SDL video, sound, joystick mapping). Key CLI
flags (see `file_help()` in `e800j_file.c`):
- `-adisk <file> [-arw]`, `-bdisk <file> [-brw]` — attach a CPCEmu `.dsk` image to drive A/B (RO unless `-arw`/`-brw` given)
- `-debug` — start in the text-console Z80 debugger/disassembler instead of free-running
- `-msg_lvl <n>` — verbosity of `msg_printf` diagnostics (see `e800j_msg.h` for levels)
- `-help` — full option list

## Architecture

The emulator is built from independent hardware-component modules that are wired together in
`e800j.c:main()` and driven by a single SDL event loop in `zx_main()` (`zx_scrn.c`). Everything
hangs off one `struct elwro` (`e800j.h`), which holds pointers to each component plus shared RAM/ROM
buffers and global timing state.

- **CPU core** (`z80_core.c`/`z80_cpu.h`/`z80_dtbl.h`/`z80_debug.c`): a from-scratch Z80 interpreter.
  `struct cpu` is intentionally a union of named register pointers/values and a flat byte array
  (`z80_data`) so the whole CPU state can be snapshotted/restored as raw bytes (used by snapshot
  loading in `e800j_file.c`). The CPU knows nothing about memory or I/O directly — it's handed function
  pointers (`memory_read_t`, `memory_write_t`, `io_read_t`, `io_write_t`) at `z80_init()` time, which is
  how `e800j_mem.c`/`e800j_io.c` get hooked in. `z80_loop()` runs a fixed number of T-states per call and
  invokes a callback (interrupt timing) partway through; the outer SDL timer (`zx_cpu_refresh`, added via
  `SDL_AddTimer` in `zx_main`) decides how often that loop runs, not the CPU itself.
- **Memory** (`e800j_mem.c`): implements paging/mapping differences between plain ZX Spectrum 48K
  (`mem_zx48_*`), ZX Spectrum 128 (`mem_zx128_*`, not fully wired up yet per `doc/ToDo`), and the Elwro's
  own extended memory map (`mem_e800j_*`, selected via `e.port_f7`/`e.mem_mask`). `main()` always
  installs `mem_e800j_read`/`mem_e800j_write` as the active handlers.
- **I/O** (`e800j_io.c`): dispatches Z80 `IN`/`OUT` port access to the keyboard, sound beeper
  (port 0xFE), FDC (`e800j_fdc.c`/`i8272_fdc.c`), and memory-paging port 0xF7.
- **Floppy disk subsystem** is two layers: `i8272_fdc.c` emulates the real NEC µPD765/i8272A controller
  chip (command phases, status registers), and `e800j_fdc.c` is the Elwro-specific glue connecting that
  chip to port I/O and drive-select register 0x175. The actual disk *image* access is abstracted one
  level further in `virt_fdd.c`/`virt_fdd.h` (`vfdd_dsk_floppy`), currently only supporting CPCEmu `.dsk`
  files — `VFDD_EMUL_DIR`/`VFDD_EMUL_REAL` emulation modes are declared but not implemented
  (see `doc/ToDo`).
- **Screen** (`zx_scrn.c`/`zx_scrn.h`): owns the SDL3 window/GL context (legacy `glDrawPixels`/`SDL_GL_SwapWindow`) and renders the ZX Spectrum
  pixel+attribute framebuffer (`screen.zx_patt`/`zx_attr`, pointers directly into `e.ram` at the
  standard Spectrum screen addresses). Also hosts the main SDL event loop (`zx_main`) that ticks the CPU,
  FDC, and screen refresh together.
- **Sound** (`e800j_sound.c`): a simple beeper emulator (port 0xFE bit) using a circular sample buffer,
  with optional sine-based anti-aliasing (`alias` config flag). AY-3-8910 sound is listed in `doc/ToDo`
  as unimplemented.
- **Keyboard** (`e800j_kbrd.c`): maps SDL key events to the ZX Spectrum/Elwro keyboard matrix read via I/O.
- **Config/CLI/snapshot loading** (`e800j_file.c`): a small custom parser for the `section { key = value; }`
  style of `e800j.cfg` (see that file for the full schema — BIOS ROM sets, CPU timing, screen/video/sound/
  joystick profiles), plus `.sna`/snapshot loading that restores CPU + RAM state directly from the raw
  `struct cpu` byte layout.
- **Portability shims** (`e800j_compat.c`/`.h`): MSVC/Windows replacement for glibc's `on_exit()` (backed by
  `atexit()`). `e800j_string.c`/`.h` is a placeholder for string helpers and is currently commented out of
  `CMakeLists.txt` (not built).
- **Messages/logging** (`e800j_msg.c`/`e800j_msg.h`): all diagnostic output goes through `msg_printf`
  with a severity level (`MSG_CRITICAL_ERROR` .. `MSG_UNVERB_MSG`); verbosity is controlled globally by
  `msg_level` (settable via `-msg_lvl`). Don't use `printf`/`fprintf` directly for emulator diagnostics —
  use `msg_printf` so verbosity filtering stays consistent.
- **Debugger** (`z80_debug.c`/`z80_debug.h`): an interactive text-console Z80 disassembler/debugger,
  entered via `-debug` or at runtime; it hooks the CPU via the `debug_t` callback in `struct cpu`.

### ROM/BIOS layout

`rom/` contains the real Elwro 800 Junior and ZX Spectrum 48K ROM images referenced by `e800j.cfg`
profile sections (`elwro_1`/`elwro_2`/`elwro_3` revisions, `spectrum48`). `e800j_prom.h` contains
PROM/decoding tables for the Elwro's custom memory-mapping logic (register 0xF7 behavior), separate
from the ROM binaries themselves.
