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

#pragma once

#include "z80_cpu.h"
#include "e800j.h"
/*
 Czyta parametry wejściowe programu poszukując opcji -f, po jej znalezieniu jako
  nazwę snapshotu uznaje następny parametr, a następnie wczytuje snapshot
*/
extern void file_get_cline(int argc, char** argv, z80* c, char* ram, char* reg373, char rest);

/*
 Wczytuje snapshot:
 c - struktura CPU Z80,
 ram - bufor pamięci ZX SPECTRUM/ JUNIOR
 reg373 - kolor ramki
 rest - jeśli TRUE, to niezgodnosć sprzętu emulowanego przerwie spowoduje wyjście z funkcji,
        wartość FALSE wygeneruje jedynie ostrzeżenie na konsoli
*/
extern int file_snap_load(const char* fname, z80* c, char* ram, char* reg373, char rest);

//extern int file_snap_save(const char *fname, z80 *c, char *ram, char *reg373);
// xx - jeśli 1 to brak opcji nie generuje błędu
extern char* file_find_cfg_option(const char* cfg, const char* name, char* str, const char* err, int lvl, int xx);

extern char* file_find_cfg_section(const char* cfg, const char* name, char* str, const char* err, int lvl);

extern char* file_get_profile(const char* cfg, char* str);

extern int file_bios_load(const char* cfg, char* rom, char* profile, int romsize);

extern char file_init(elwro*, char* cpath, int argc, char** argv);

/*  przeszukuje parametry wejściowe wywołania w poszukiwaniu opcji,
    jeśli znajdzie zwraca jej numer, inaczej zwraca 0
*/
extern int file_lookup_option(int argc, char** argv, const char* name);

extern void file_option_param(int argc, char** argv, int pos, int min, int max, int* val);

extern void file_help();