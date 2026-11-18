/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Copyright (C) 2026 Artur Wojcik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "e800j_compat.h"

#if defined(_MSC_VER) || defined(__APPLE__) || (defined(_WIN32) && !defined(__GNUC__))

#include <stdlib.h>

#define ON_EXIT_MAX_HANDLERS 16

static void (*on_exit_funcs[ON_EXIT_MAX_HANDLERS])(int, void*);
static void* on_exit_args[ON_EXIT_MAX_HANDLERS];
static int on_exit_count = 0;

static void on_exit_run_handlers(void) {
    /* run in reverse registration order, like atexit() does */
    for (int i = on_exit_count - 1; i >= 0; i--)
        on_exit_funcs[i](0, on_exit_args[i]);
}

int on_exit(void (*function)(int, void*), void* arg) {
    if (on_exit_count >= ON_EXIT_MAX_HANDLERS)
        return -1;
    if (on_exit_count == 0)
        atexit(on_exit_run_handlers);
    on_exit_funcs[on_exit_count] = function;
    on_exit_args[on_exit_count] = arg;
    on_exit_count++;
    return 0;
}

#endif

#ifdef _WIN32

#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "e800j_config.h"

/* strumienie standardowe przekierowane przez uruchamiającego (potok/plik), np. elwro --debug < cmds.txt */
static bool win_in_redirected;
static bool win_out_redirected;
static bool win_own_console; /* --debug otworzył własną konsolę */

static bool win_std_redirected(const DWORD id) {
    const HANDLE h = GetStdHandle(id);
    if (h == nullptr || h == INVALID_HANDLE_VALUE) {
        return false;
    }
    const DWORD type = GetFileType(h);
    return type == FILE_TYPE_PIPE || type == FILE_TYPE_DISK;
}

/* kieruje nieprzekierowane strumienie na urządzenia o podanych nazwach (konsola albo "NUL") */
static void win_reopen_streams(const char* in_name, const char* out_name) {
    if (!win_out_redirected) {
        (void)freopen(out_name, "w", stdout);
        (void)freopen(out_name, "w", stderr);
    }
    if (!win_in_redirected) {
        (void)freopen(in_name, "r", stdin);
    }
}

bool compat_console_attach(void) {
    win_in_redirected = win_std_redirected(STD_INPUT_HANDLE);
    win_out_redirected = win_std_redirected(STD_OUTPUT_HANDLE);
    const bool attached = AttachConsole(ATTACH_PARENT_PROCESS);
    if (attached) {
        win_reopen_streams("CONIN$", "CONOUT$");
    }
    return !attached && !win_out_redirected;
}

/*
 Terminal nadrzędny nie czeka na aplikację GUI (znak zachęty wraca od razu i walczy z debuggerem o wejście),
 więc debugger dostaje własną konsolę (okno "Elwro 800 Junior - debugger"); terminal pozostaje sprawny,
 a zamrożone jest tylko okno emulatora. Przy przekierowanym wejściu i wyjściu nic się nie zmienia.
*/
void compat_debug_console(void) {
    if (win_own_console || (win_in_redirected && win_out_redirected)) {
        return;
    }
    FreeConsole();
    if (!AllocConsole()) {
        return;
    }
    SetConsoleTitleA("Elwro 800 Junior - debugger");
    win_reopen_streams("CONIN$", "CONOUT$");
    win_own_console = true;
}

void compat_debug_console_close(void) {
    if (!win_own_console) {
        return;
    }
    win_own_console = false;
    FreeConsole();
    win_reopen_streams("NUL", "NUL");
}

#else

#include <unistd.h>

bool compat_console_attach(void) {
    return isatty(STDERR_FILENO) == 0;
}

void compat_debug_console(void) {
}

void compat_debug_console_close(void) {
}

#endif
