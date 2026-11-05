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
#include "zx_scrn.h"
#include "e800j_sound.h"
#include "e800j_kbrd.h"
#include "e800j_fdc.h"

typedef struct {
    /* elementy komputera */
    z80* cpu;
    struct screen* zx;
    kbrd* kb;
    sound* snd;
    e800j_fdc* fdc;
    /* pamięć */
    char* ram;
    char* rom;
    int ram_size;
    int rom_size;

    char tcpu;
    void* t1;
    int cpu_rfsh;
    int ifreq; /* co ile taktów CPU ma zostać wywołane przerwanie */
    int tr; /* ilość taktów/wywołanie   */
    /* porty I/O */
    uchar port_f7; /* port sterowania pamięcią */
    uchar port_fe; /* port głośniczka, koloru ramki, wyj magnetofonu */
    /* inne */
    ushort mem_mask; /* maska ostatnich 3 bitów adresu PROM 'MEM' - A6,A7,A8 */
    /* JUNET */
    ushort junet_addr; /* adres komputera w sieci junet */

    char debug; /* true -> uruchamia w trybie komend */
} elwro;

extern void zx_main(elwro*);