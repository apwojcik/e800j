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

static void msg_putchar(const int chr) {
    if (msg_stream == 0)
        putchar(chr);
}

static void msg_putstr(const char* str) {
    msg_putchar(*str);
}

static const char* msg[] = {"(E!)", "(EE)", "(WW)", "(II)", "(VI)", "(NI)", "(UI)"};

void msg_printf(char level, const char* fmt, ...) {
    va_list ap;
    char* p;
    int ival;
    char tabl[16] = "";

    if (level > msg_level)
        return;
    if (level > 6)
        level = 6;
    msg_putstr((char*)msg[(int)level]);

    va_start(ap, fmt);

    for (p = (char*)fmt; *p; p++) {
        if (*p != '%') {
            msg_putchar(*p);
            continue;
        }
        switch (*(++p)) {
        case 's':
            for (char* sval = va_arg(ap, char*); *sval; sval++)
                msg_putchar(*sval);
            break;
        case 'c':
            ival = va_arg(ap, int);
            msg_putchar((char)ival);
            break;
        case 'i':
            ival = va_arg(ap, int);
            sprintf(tabl, "%i", ival);
            msg_putstr(tabl);
            break;
        case 'x':
            ival = va_arg(ap, int);
            sprintf(tabl, "0x%x", ival);
            msg_putstr(tabl);
            break;
        }
    }
    va_end(ap);
}
