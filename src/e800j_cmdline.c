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

enum opt_id {
    OPT_HELP, OPT_CONFIG, OPT_PROFILE, OPT_FULLSCREEN, OPT_NOFRAME, OPT_BPP, OPT_XRES, OPT_YRES,
    OPT_DEBUG, OPT_ADISK, OPT_BDISK, OPT_LOGLEVEL, OPT_AREADWRITE, OPT_BREADWRITE
};

/* Tabela opcji: krótka nazwa (0 = brak), długa nazwa (nullptr = brak), czy wymaga wartości */
static const struct {
    enum opt_id id;
    char short_name;
    const char* long_name;
    bool value;
} options[] = {
    { OPT_HELP,        'h', "help",           false },
    { OPT_CONFIG,      'c', "config",         true  },
    { OPT_PROFILE,     'p', "profile",        true  },
    { OPT_FULLSCREEN,  'f', "full-screen",    false },
    { OPT_NOFRAME,     'n', "no-frame",       false },
    { OPT_BPP,         0,   "bits-per-pixel", true  },
    { OPT_XRES,        'x', nullptr,          true  },
    { OPT_YRES,        'y', nullptr,          true  },
    { OPT_DEBUG,       0,   "debug",          false },
    { OPT_ADISK,       'A', "a-disk",         true  },
    { OPT_BDISK,       'B', "b-disk",         true  },
    { OPT_LOGLEVEL,    0,   "log-level",      true  },
    { OPT_AREADWRITE,  'a', "a-read-write",   false },
    { OPT_BREADWRITE,  'b', "b-read-write",   false },
};

static const int option_count = (int)(sizeof options / sizeof options[0]);

static int cmdline_find_short(const char c) {
    for (int k = 0; k < option_count; k++)
        if (options[k].short_name != 0 && options[k].short_name == c)
            return k;
    return -1;
}

static int cmdline_find_long(const char* name, const size_t len) {
    for (int k = 0; k < option_count; k++)
        if (options[k].long_name != nullptr && strlen(options[k].long_name) == len &&
            strncmp(options[k].long_name, name, len) == 0)
            return k;
    return -1;
}

bool cmdline_parse(struct cmdline* cl, const int argc, const char** argv) {
    memset(cl, 0, sizeof(*cl));
    cl->bpp = cl->xres = cl->yres = cl->msg_level = CMDLINE_UNSET;

    bool options_end = false;
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        int k = -1;
        const char* name = nullptr; /* nazwa opcji bez prefiksu (do '=' lub końca) */

        if (!options_end && strcmp(a, "--") == 0) {
            options_end = true;
            continue;
        }
        if (!options_end && a[0] == '-' && a[1] == '-') {
            /* opcja długa: --nazwa lub --nazwa=wartość */
            name = a + 2;
            const char* eq = strchr(name, '=');
            k = cmdline_find_long(name, eq != nullptr ? (size_t)(eq - name) : strlen(name));
            if (k < 0) {
                msg_printf(MSG_CRITICAL_ERROR, "Unknown option: %s (try --help)\n", a);
                return false;
            }
        } else if (!options_end && a[0] == '-' && a[1] != '\0') {
            /* flaga krótka: -x (dokładnie jedna litera) */
            k = a[2] == '\0' ? cmdline_find_short(a[1]) : -1;
            if (k < 0) {
                msg_printf(MSG_CRITICAL_ERROR, "Unknown option: %s (long options start with --, try --help)\n", a);
                return false;
            }
            name = a + 1;
        }
#ifdef _WIN32
        else if (!options_end && a[0] == '/' && a[1] != '\0') {
            /* Windows: /x lub /nazwa[=wartość]; nieznane /... to ścieżka (argument pozycyjny) */
            name = a + 1;
            const char* eq = strchr(name, '=');
            const size_t len = eq != nullptr ? (size_t)(eq - name) : strlen(name);
            k = len == 1 ? cmdline_find_short(name[0]) : cmdline_find_long(name, len);
        }
#endif

        if (k < 0) {
            /* argument pozycyjny */
            if (cl->snapshot != nullptr) {
                msg_printf(MSG_CRITICAL_ERROR, "Unexpected argument: %s (snapshot already given: %s)\n", a, cl->snapshot);
                return false;
            }
            cl->snapshot = a;
            continue;
        }

        /* wartość opcji: po '=' (tylko opcje długie) lub w następnym argumencie */
        const char* eq = strchr(name, '=');
        const bool is_long = strlen(name) > 1 && !(a[0] == '-' && a[1] != '-');
        const char* value = nullptr;
        if (options[k].value) {
            if (eq != nullptr && is_long) {
                value = eq + 1;
            } else if (i + 1 < argc) {
                value = argv[++i];
            } else {
                msg_printf(MSG_CRITICAL_ERROR, "Missing an argument for the %s option.\n", a);
                return false;
            }
        } else if (eq != nullptr && is_long) {
            msg_printf(MSG_CRITICAL_ERROR, "Option %s does not take a value.\n", a);
            return false;
        }

        switch (options[k].id) {
        case OPT_HELP:
            cl->help = true;
            return true;
        case OPT_CONFIG:      cl->cfg = value; break;
        case OPT_PROFILE:     cl->profile = value; break;
        case OPT_FULLSCREEN:  cl->fullscreen = true; break;
        case OPT_NOFRAME:     cl->noframe = true; break;
        case OPT_DEBUG:       cl->debug = true; break;
        case OPT_ADISK:       cl->disk[0] = value; break;
        case OPT_BDISK:       cl->disk[1] = value; break;
        case OPT_AREADWRITE:  cl->disk_rw[0] = true; break;
        case OPT_BREADWRITE:  cl->disk_rw[1] = true; break;
        case OPT_BPP:
            if (!cmdline_ranged(a, value, 0, 32, &cl->bpp))
                return false;
            break;
        case OPT_XRES:
            if (!cmdline_ranged(a, value, 256, 2048, &cl->xres))
                return false;
            break;
        case OPT_YRES:
            if (!cmdline_ranged(a, value, 256, 2048, &cl->yres))
                return false;
            break;
        case OPT_LOGLEVEL:
            if (!cmdline_ranged(a, value, 0, 6, &cl->msg_level))
                return false;
            break;
        }
    }
    return true;
}
