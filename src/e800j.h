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

#include "e800j_config.h"

struct screen;
struct cpu;
struct keyboard;
struct sound;
struct fdc;
struct vfdd_disk;

struct elwro {
    // elementy komputera
    struct screen* scn;
    struct cpu* cpu;
    struct keyboard* kbd;
    struct sound* snd;
    struct fdc* fdc;
    struct vfdd_disk* disk[2]; /* napędy A i B */
    char diskdir[512]; /* katalog z obrazami *.dsk dla F5/F6, pusty -> okno wyboru pliku */
    char snap_path[512]; /* snapshot podany w linii poleceń (dla Alt+F12), pusty -> brak */

    // pamięć
    unsigned char* ram;
    unsigned char* rom;
    int ram_size;
    int rom_size;

    bool tcpu;
    unsigned int t1;
    unsigned int cpu_refresh;
    int ifreq; /* co ile taktów CPU ma zostać wywołane przerwanie */
    int tr; /* ilość taktów/wywołanie */

    // porty I/O
    unsigned char port_f7; /* port sterowania pamięcią */
    unsigned char port_fe; /* port głośniczka, koloru ramki, wyjścia magnetofonu */

    // inne
    unsigned short mem_mask; /* maska ostatnich 3 bitów adresu PROM 'MEM' - A6,A7,A8 */

    // JUNET
    unsigned short junet_addr; /* adres komputera w sieci junet */

    char debug; /* true -> uruchamia w trybie komend */
};

/* przycisk RESET: CPU, mapowanie pamięci (port 0xF7) i kontroler FDC; zawartość RAM zostaje */
void elwro_reset(struct elwro*);

/* zimny restart: jak świeży start emulatora (RAM wypełniony losowo, jak po włączeniu zasilania) + reset */
void elwro_cold_reset(struct elwro*);

/* zimny restart + ponowne wczytanie snapshotu z linii poleceń; zwraca 0 gdy wczytano, != 0 gdy brak snapshotu lub błąd */
int elwro_snap_restart(struct elwro*);
/* zimny restart + wczytanie snapshotu z podanej ścieżki (nullptr/pusta -> sam zimny restart); zwraca 0 gdy wczytano */
int elwro_restart_snap(struct elwro*, const char* path);
/* kolejkuje komunikat o wczytanym snapshocie (sama nazwa pliku), błędzie lub - dla pustej ścieżki - braku snapshotu;
   wołać po komunikacie o dyskietkach, error = wynik file_snap_load */
void elwro_snap_announce(struct elwro*, const char* path, int error);
int zx_main(struct elwro*);
