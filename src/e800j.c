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
#include "virt_fdd.h"
#include "e800j_msg.h"

static struct screen screen;

static void disk_vlookup(int argc, char** argv, char* dparm, char* mode, struct vfdd_disk* disk, char dn) {
    int tmp;
    if ((tmp = file_lookup_option(argc, argv, dparm)) != 0) {
        if (tmp < argc) {
            if (file_lookup_option(argc, argv, mode) != 0) {
                if (vfdd_floppy(disk, VFDD_LOAD, VFDD_RW, 0, 0, argv[tmp + 1]) < 0) {
                    msg_printf(MSG_ERROR, "Opening file %s failed.\n", argv[tmp + 1]);
                }
                msg_printf(MSG_MSG, "Open file %s for RW as drive %c.\n", argv[tmp + 1], dn);
            } else {
                if (vfdd_floppy(disk, VFDD_LOAD, VFDD_RO, 0, 0, "mat.dsk") < 0) {
                    msg_printf(MSG_ERROR, "Opening file %s failed.\n", argv[tmp + 1]);
                }
       	        msg_printf(MSG_MSG, "Open file %s for RO as drive %c.\n", argv[tmp + 1], dn);
            }
        }
    }
}

int main(int argc, char** argv) {
    struct elwro e;
    struct fdc fdc;
    struct cpu cpu;
    struct dbg dbg;
    struct sound snd;
    struct keyboard kbd;
    struct vfdd_disk disk_a, disk_b;
    int tmp;

    e.cpu = &cpu;
    e.scn = &screen;
    e.snd = &snd;
    e.kbd = &kbd;
    e.fdc = &fdc;
    e.mem_mask = 0;
    e.junet_addr = 0xf3;
    e.debug = 0;

    if (file_lookup_option(argc, argv, "-help"))
        file_help();
    if (file_lookup_option(argc, argv, "-h"))
        file_help();
    if (file_lookup_option(argc, argv, "--help"))
        file_help();
    if ((tmp = file_lookup_option(argc, argv, "-msg_lvl")))
        msg_level = strtol(argv[tmp + 1], NULL, 0);

    vfdd_floppy(&disk_a, VFDD_INIT, 0, 0, 0, 0);
    vfdd_floppy(&disk_b, VFDD_INIT, 0, 0, 0, 0);

    disk_vlookup(argc, argv, "-adisk", "-arw", &disk_a, 'A');
    disk_vlookup(argc, argv, "-bdisk", "-brw", &disk_b, 'B');

    /* odczyt konfiguracji i na tej podstawie ustawienie e800 */

    if (file_init(&e, "./e800j.cfg", argc, argv))
        return -1;

    screen.zx_patt = e.ram + 16384;
    screen.zx_attr = e.ram + 16384 + 6 * 1024;
    screen_border(&screen, 1);

    key_init(&kbd);
    z80_init(&cpu, mem_e800j_read, mem_e800j_write, io_read, io_write, &e, NULL);
    dbg_init(&dbg, &cpu, e.debug, 1);

    fdc_init(&fdc);
    i8272_fdd_connect(&fdc.i8272, vfdd_drive, &disk_a, &disk_b, 0, 0);

    screen_init(&screen, screen.width, screen.height, screen.bpp, argc, argv, 20);

    snd_init(&snd, &cpu);
    file_get_cline(argc, argv, &e, e.ram, &e.scn->border_color, 0);

    zx_main(&e);

    snd_free_buffers(&snd);
    vfdd_floppy(&disk_a, VFDD_UNLOAD, 0, 0, 0, 0);
    vfdd_floppy(&disk_b, VFDD_UNLOAD, 0, 0, 0, 0);
    return 0;
}
