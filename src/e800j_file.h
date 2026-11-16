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

struct elwro;

/* Nazwa snapshotu z linii poleceń: opcja -snap <plik> lub jedyny parametr pozycyjny; nullptr gdy brak */
const char* file_snap_cline(int argc, const char** argv);

/*
 Wczytuje snapshot podany w linii poleceń (patrz file_snap_cline), jeśli go podano
*/
void file_get_cline(int argc, const char** argv, const struct elwro* e, unsigned char* ram, unsigned char* r373, char rest);

/*
 Wczytuje snapshot:
 c - struktura CPU Z80,
 ram - bufor pamięci ZX SPECTRUM/ JUNIOR
 reg373 - kolor ramki
 rest - jeśli TRUE, to niezgodnosć sprzętu emulowanego przerwie spowoduje wyjście z funkcji,
        wartość FALSE wygeneruje jedynie ostrzeżenie na konsoli
*/
int file_snap_load(const char* filename, const struct elwro* e, unsigned char* ram, unsigned char* r373, char rest);

/*
 Zapisuje snapshot .sna (48K): rejestry CPU, RAM 0x4000-0xFFFF i kolor ramki.
 Stan portu 0xF7 (mapowanie pamięci) nie jest zapisywany.
*/
int file_snap_save(const char* filename, const struct elwro* e);
// xx - jeśli 1 to brak opcji nie generuje błędu
char* file_find_cfg_option(const char* cfg, const char* name, char* str, const char* err, int lvl, int xx);
char* file_find_cfg_section(const char* cfg, const char* name, char* str, const char* err, int lvl);

char* file_get_profile(const char* cfg, char* profile);

int file_bios_load(const char* cfg, const unsigned char* rom, const char* profile, int rom_size);

bool file_init(struct elwro*, const char* config_path, int argc, const char** argv);

/*  przeszukuje parametry wejściowe wywołania w poszukiwaniu opcji,
    jeśli znajdzie zwraca jej numer, inaczej zwraca 0
*/
int file_lookup_option(int argc, const char** argv, const char* name);

void file_option_param(int argc, const char** argv, int pos, int min, int max, int* val);

void file_help(void);
