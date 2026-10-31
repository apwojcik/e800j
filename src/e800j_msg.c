/* Elwro 800 Junior emulator v 0.2
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Email: krzkomar@wp.pl
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

#include <stdarg.h>
#include <stdio.h>

#include "e800j_msg.h"

char msg_stream = 0;
long msg_level = 0;

static void msg_putchar(const char chr) {
    if (msg_stream == 0) {
        putchar(chr);
    }
}

static void msg_putstr(const char* str) {
    while (*str != 0) {
        msg_putchar(*str++);
    }
}

static void msg_putfloat(const float value) {
    char t[16] = "";
    sprintf(t, "%f", value);
    msg_putstr(t);
}

static void msg_putint(const int value) {
    char t[16] = "";
    sprintf(t, "%i", value);
    msg_putstr(t);
}

static void msg_putuint(const unsigned int value) {
    char t[16] = "";
    sprintf(t, "%u", value);
    msg_putstr(t);
}

static void msg_puthex(const unsigned int value) {
    char t[16] = "";
    sprintf(t, "%x", value);
    msg_putstr(t);
}

static void msg_putaddr(const unsigned int value) {
    char t[16] = "";
    sprintf(t, "%04x", value);
    msg_putstr(t);
}

static const char* msg[] = {
    "(E!) ", "(EE) ", "(WW) ", "(II) ", "(VI) ", "(NI) ", "(UI) "
};

void msg_printf(int level, const char* fmt, ...) {
    if (level <= msg_level) {
        va_list ap;
        if (level > 6) {
            level = 6;
        }
        msg_putstr(msg[level]);
        va_start(ap, fmt);
        for (const char* p = (char*)fmt; *p; ++p) {
            if (*p != '%') {
                msg_putchar(*p);
                continue;
            }
            switch (*(++p)) {
            case 's':
                msg_putstr(va_arg(ap, const char *));
                break;
            case 'c':
                msg_putchar(va_arg(ap, int));
                break;
            case 'i':
                msg_putint(va_arg(ap, int));
                break;
            case 'u':
                msg_putuint(va_arg(ap, unsigned int));
                break;
            case 'x':
                msg_puthex(va_arg(ap, unsigned int));
                break;
            case 'X':
                msg_putaddr(va_arg(ap, unsigned int));
                break;
            case 'f':
                msg_putfloat(va_arg(ap, double));
                break;
            default:
                break;
            }
        }
        va_end(ap);
        fflush(stdout);
    }
}
