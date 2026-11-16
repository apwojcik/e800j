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

#pragma once

#include <stdbool.h>

/* Wartość "nie podano" dla opcji liczbowych */
#define CMDLINE_UNSET (-1)

/*
 Opcje i argumenty linii poleceń po jednokrotnym przetworzeniu w main().
 Napisy wskazują na argv, więc struktura jest ważna tak długo jak argv.
*/
struct cmdline {
    const char* snapshot;     /* argument pozycyjny: snapshot wczytywany przy starcie, nullptr gdy brak */
    const char* cfg;          /* -cfg: plik konfiguracji, nullptr = domyślny */
    const char* profile;      /* -profile: nazwa profilu, nullptr = z sekcji 'main' */
    const char* disk[2];      /* -adisk/-bdisk: obraz dyskietki w napędzie A/B, nullptr gdy brak */
    bool disk_rw[2];          /* -arw/-brw: otwarcie obrazu do zapisu */
    bool help;                /* -h, -help, --help */
    bool debug;               /* -debug */
    bool fullscreen;          /* -fs */
    bool noframe;             /* -nf */
    int bpp;                  /* -bpp (0..32) lub CMDLINE_UNSET */
    int xres;                 /* -xres (256..2048) lub CMDLINE_UNSET */
    int yres;                 /* -yres (256..2048) lub CMDLINE_UNSET */
    int msg_level;            /* -msg_lvl lub CMDLINE_UNSET */
};

/*
 Przetwarza argv (bez wywoływania exit). Zwraca false i wypisuje komunikat (msg_printf)
 przy nieznanej opcji, braku lub błędnej wartości opcji albo nadmiarowym argumencie.
 Po napotkaniu opcji pomocy ustawia 'help' i kończy przetwarzanie.
*/
bool cmdline_parse(struct cmdline* cl, int argc, const char** argv);
