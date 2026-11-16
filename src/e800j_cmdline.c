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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "e800j_config.h"
#include "e800j_cmdline.h"
#include "e800j_msg.h"

/* Wartość liczbowa opcji; false (z komunikatem) gdy to nie jest liczba. */
static bool cmdline_number(const char* opt, const char* text, long* val) {
    char* end;
    errno = 0;
    *val = strtol(text, &end, 0);
    if (end == text || *end != '\0' || errno != 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Invalid number for option %s: %s\n", opt, text);
        return false;
    }
    return true;
}

/* Wartość liczbowa z zakresu min..max; poza zakresem to błąd. */
static bool cmdline_ranged(const char* opt, const char* text, const int min, const int max, int* out) {
    long val;
    if (!cmdline_number(opt, text, &val))
        return false;
    if (val < min || val > max) {
        msg_printf(MSG_CRITICAL_ERROR, "Parameter for option %s out of range: %i (allowed %i..%i)\n", opt, (int)val, min, max);
        return false;
    }
    *out = (int)val;
    return true;
}

bool cmdline_parse(struct cmdline* cl, const int argc, const char** argv) {
    memset(cl, 0, sizeof(*cl));
    cl->bpp = cl->xres = cl->yres = cl->msg_level = CMDLINE_UNSET;

    bool options_end = false;
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];

        if (options_end || a[0] != '-' || a[1] == '\0') {
            if (cl->snapshot != nullptr) {
                msg_printf(MSG_CRITICAL_ERROR, "Unexpected argument: %s (snapshot already given: %s)\n", a, cl->snapshot);
                return false;
            }
            cl->snapshot = a;
            continue;
        }
        if (strcmp(a, "--") == 0) {
            options_end = true;
            continue;
        }
        if (strcmp(a, "-h") == 0 || strcmp(a, "-help") == 0 || strcmp(a, "--help") == 0) {
            cl->help = true;
            return true;
        }

        /* opcje bez wartości */
        if (strcmp(a, "-debug") == 0) {
            cl->debug = true;
            continue;
        }
        if (strcmp(a, "-fs") == 0) {
            cl->fullscreen = true;
            continue;
        }
        if (strcmp(a, "-nf") == 0) {
            cl->noframe = true;
            continue;
        }
        if (strcmp(a, "-arw") == 0) {
            cl->disk_rw[0] = true;
            continue;
        }
        if (strcmp(a, "-brw") == 0) {
            cl->disk_rw[1] = true;
            continue;
        }

        /* opcje z wartością */
        const char** str = nullptr;
        int* num = nullptr;
        int min = 0, max = 0;
        if (strcmp(a, "-cfg") == 0)
            str = &cl->cfg;
        else if (strcmp(a, "-profile") == 0)
            str = &cl->profile;
        else if (strcmp(a, "-adisk") == 0)
            str = &cl->disk[0];
        else if (strcmp(a, "-bdisk") == 0)
            str = &cl->disk[1];
        else if (strcmp(a, "-bpp") == 0)
            num = &cl->bpp, min = 0, max = 32;
        else if (strcmp(a, "-xres") == 0)
            num = &cl->xres, min = 256, max = 2048;
        else if (strcmp(a, "-yres") == 0)
            num = &cl->yres, min = 256, max = 2048;
        else if (strcmp(a, "-msg_lvl") == 0)
            num = &cl->msg_level, min = 0, max = 6;
        else {
            msg_printf(MSG_CRITICAL_ERROR, "Unknown option: %s (try -help)\n", a);
            return false;
        }

        if (i + 1 >= argc) {
            msg_printf(MSG_CRITICAL_ERROR, "Missing an argument for the %s option.\n", a);
            return false;
        }
        const char* value = argv[++i];
        if (str != nullptr)
            *str = value;
        else if (!cmdline_ranged(a, value, min, max, num))
            return false;
    }
    return true;
}
