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

#include <stdio.h>
#include <stdlib.h>

#include "z80_cpu.h"
#include "z80_debug.h"
#include "zx_scrn.h"
#include "e800j.h"
#include "e800j_file.h"
#include "e800j_kbrd.h"
#include "e800j_sound.h"
#include "e800j_mem.h"
#include "e800j_io.h"
#include "e800j_fdc.h"
#include "e800j_disk.h"
#include "virt_fdd.h"
#include "e800j_msg.h"

static struct screen screen;

static void disk_vlookup(int argc, const char** argv, char* dparm, char* mode, struct vfdd_disk* disk, char dn) {
    const int tmp = file_lookup_option(argc, argv, dparm);
    if (tmp != 0) {
        if (tmp < argc) {
            if (file_lookup_option(argc, argv, mode) != 0) {
                if (vfdd_dsk_floppy(disk, VFDD_LOAD, VFDD_RW, 0, 0, argv[tmp + 1]) < 0) {
                    msg_printf(MSG_ERROR, "Opening file %s failed.\n", argv[tmp + 1]);
                }
                msg_printf(MSG_MSG, "Open file %s for RW as drive %c.\n", argv[tmp + 1], dn);
            } else {
                if (vfdd_dsk_floppy(disk, VFDD_LOAD, VFDD_RO, 0, 0, argv[tmp + 1]) < 0) {
                    msg_printf(MSG_ERROR, "Opening file %s failed.\n", argv[tmp + 1]);
                }
       	        msg_printf(MSG_MSG, "Open file %s for RO as drive %c.\n", argv[tmp + 1], dn);
            }
        }
    }
}

void elwro_reset(struct elwro* e) {
    struct cpu* z = e->cpu;
    /* liczniki taktów i flagi debuggera nie są stanem CPU - zerowanie ticks rozsynchronizowałoby dźwięk i ramkę */
    const long ticks = z->ticks;
    const int call_cnt = z->call_cnt;
    const bool int_off = z->int_off;
    const bool jmp_off = z->jmp_off;
    const unsigned char debug_on = z->debug_on;
    const unsigned char cpu_off = z->cpu_off;
    z80_reset(z);
    z->ticks = ticks;
    z->call_cnt = call_cnt;
    z->int_off = int_off;
    z->jmp_off = jmp_off;
    z->debug_on = debug_on;
    z->cpu_off = cpu_off;
    /* mapowanie pamięci i ekran wracają do stanu po włączeniu zasilania; RAM pozostaje nietknięty */
    e->port_f7 = 0;
    e->mem_mask = 0;
    e->scn->zx_patt = e->ram + 0x4000;
    e->scn->zx_attr = e->ram + 0x5800;
    fdc_reset(e->fdc);
    msg_printf(MSG_MSG, "Reset\n");
    screen_toast(e->scn, "Reset");
}

void elwro_cold_reset(struct elwro* e) {
    /* jak po włączeniu zasilania: RAM zawiera losowe dane (tak jak w file_init) */
    for (int i = 0; i < e->ram_size; i++) {
        e->ram[i] = rand() & 0xff;
    }
    elwro_reset(e);
    screen_border(e->scn, 1);
    msg_printf(MSG_MSG, "Cold restart\n");
    screen_toast(e->scn, "Cold restart");
}

int elwro_snap_restart(struct elwro* e) {
    if (e->snap_path[0] == 0) {
        elwro_cold_reset(e);
        return 1;
    }
    elwro_cold_reset(e);
    if (file_snap_load(e->snap_path, e, e->ram, &e->scn->border_color, 0) != 0)
        return 1;
    screen_toast(e->scn, "Snapshot restarted: %s", e->snap_path);
    return 0;
}

int main(int argc, const char** argv) {
    struct elwro e;
    int tmp;
    struct fdc fdc;
    struct cpu cpu;
    struct dbg dbg;
    struct sound snd;
    struct keyboard kbd;
    struct vfdd_disk disk_a, disk_b;

    e.cpu = &cpu;
    e.scn = &screen;
    e.snd = &snd;
    e.kbd = &kbd;
    e.fdc = &fdc;
    e.disk[0] = &disk_a;
    e.disk[1] = &disk_b;
    e.diskdir[0] = 0;
    e.snap_path[0] = 0;
    if (argc == 2 && *argv[1] != '-')
        snprintf(e.snap_path, sizeof(e.snap_path), "%s", argv[1]);
    screen.toast_ms = 10000;
    e.mem_mask = 0;
    e.junet_addr = 0xf3;
    e.debug = 0;

    if (file_lookup_option(argc, argv, "-help") ||
        file_lookup_option(argc, argv, "-h") ||
        file_lookup_option(argc, argv, "--help"))
    {
        file_help();
    }

    if ((tmp = file_lookup_option(argc, argv, "-msg_lvl")) != 0 && tmp + 1 < argc) {
        msg_level = strtol(argv[tmp + 1], nullptr, 0);
    }

    vfdd_dsk_floppy(&disk_a, VFDD_INIT, 0, 0, 0, nullptr);
    vfdd_dsk_floppy(&disk_b, VFDD_INIT, 0, 0, 0, nullptr);

    disk_vlookup(argc, argv, "-adisk", "-arw", &disk_a, 'A');
    disk_vlookup(argc, argv, "-bdisk", "-brw", &disk_b, 'B');

    /* odczyt konfiguracji i na tej podstawie ustawienie e800 */

    if (file_init(&e, "e800j.cfg", argc, argv) == false) {
        return -1;
    }

    screen.zx_patt = e.ram + 16384;
    screen.zx_attr = e.ram + 16384 + 6 * 1024;
    screen_border(&screen, 1);

    key_init(&kbd);
    z80_init(&cpu, mem_e800j_read, mem_e800j_write, io_read, io_write, &e, nullptr);
    dbg_init(&dbg, &cpu, e.debug, 1);

    fdc_init(&fdc);
    i8272_fdd_connect(&fdc.i8272, vfdd_drive, &disk_a, &disk_b, nullptr, nullptr);

    if (screen_init(&screen, screen.width, screen.height, screen.bpp, argc, argv, e.scn->rfsh) == false) {
        return -1;
    }

    snd_init(&snd, &cpu, (double)e.tr * 1000.0 / (double)e.cpu_refresh);
    file_get_cline(argc, argv, &e, e.ram, &e.scn->border_color, 0);

    disk_announce(&e);
    zx_main(&e);

    snd_free_buffers(&snd);
    vfdd_dsk_floppy(&disk_a, VFDD_UNLOAD, 0, 0, 0, nullptr);
    vfdd_dsk_floppy(&disk_b, VFDD_UNLOAD, 0, 0, 0, nullptr);
    return 0;
}
