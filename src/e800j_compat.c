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

#else

/* ISO C zabrania pustej jednostki translacji */
typedef int e800j_compat_unused_t;

#endif
