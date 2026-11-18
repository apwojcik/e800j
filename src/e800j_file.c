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
#include <string.h>
#include <libspectrum.h>
#include "e800j_file.h"
#include "e800j.h"
#include "e800j_kbrd.h"
#include "z80_cpu.h"
#include "e800j_msg.h"
#include "e800j_sound.h"
#include "zx_scrn.h"

static int file_snap_cpy(libspectrum_snap* snap, const struct elwro* e, unsigned char* ram, unsigned char* reg373, const char rest) {
    if (snap == NULL || e == NULL || ram == NULL || reg373 == NULL)
        return 1;

    const libspectrum_machine machine = libspectrum_snap_machine(snap);

    if ((machine != LIBSPECTRUM_MACHINE_48) && (machine != LIBSPECTRUM_MACHINE_16)) {
        msg_printf(MSG_WARN, "Nieobsługiwany typ komputera: %s\n", libspectrum_machine_name(machine));
        if (rest) {
            return 0;
        }
    }

    /* ustawienie rejestrów */
    elwro_cpu_reset((struct elwro*)e);

    e->cpu->As0 = libspectrum_snap_a(snap);
    e->cpu->Fs0 = libspectrum_snap_f(snap);
    e->cpu->As1 = libspectrum_snap_a_(snap);
    e->cpu->Fs1 = libspectrum_snap_f_(snap);

    e->cpu->BCs0 = libspectrum_snap_bc(snap);
    e->cpu->BCs1 = libspectrum_snap_bc_(snap);
    e->cpu->DEs0 = libspectrum_snap_de(snap);
    e->cpu->DEs1 = libspectrum_snap_de_(snap);
    e->cpu->HLs0 = libspectrum_snap_hl(snap);
    e->cpu->HLs1 = libspectrum_snap_hl_(snap);

    e->cpu->IXs = libspectrum_snap_ix(snap);
    e->cpu->IYs = libspectrum_snap_iy(snap);
    e->cpu->Is = libspectrum_snap_i(snap);
    e->cpu->Rs = libspectrum_snap_r(snap);
    e->cpu->SPs = libspectrum_snap_sp(snap);
    e->cpu->PCs = libspectrum_snap_pc(snap);

    e->cpu->iff1 = libspectrum_snap_iff1(snap);
    e->cpu->iff2 = libspectrum_snap_iff2(snap);
    e->cpu->imode = libspectrum_snap_im(snap);
    e->cpu->halt = libspectrum_snap_halted(snap) != 0;

    /* kolor ramki */
    *reg373 = libspectrum_snap_out_ula(snap) & 0xff;

    /* kopiowanie pamięci */
    for (int i = 0; i < 16; i++)
        if (libspectrum_snap_pages(snap, i)) {
            switch (i) {
            case 0:
                memcpy(ram + 0xc000, libspectrum_snap_pages(snap, i), 0x4000);
                break;
            case 2:
                memcpy(ram + 0x8000, libspectrum_snap_pages(snap, i), 0x4000);
                break;
            case 5:
                memcpy(ram + 0x4000, libspectrum_snap_pages(snap, i), 0x4000);
                break;
            default:
                break;
            }
        }
    return 0;
}

static int file_load_blk(FILE* f, unsigned char* buffer, const long length) {
    return fread(buffer, length, 1, f) != 1;
}

int file_snap_load(const char* filename, const struct elwro* e, unsigned char* ram, unsigned char* r373, const char rest) {
    struct stat s;
    if (stat(filename, &s) != 0) {
        msg_printf(MSG_ERROR, "Snapshot file doesn't exists: %s", filename);
        return 1;
    }
    unsigned char* buffer;
    FILE* f;
    if ((f = fopen(filename, "rb")) == nullptr) {
        msg_printf(MSG_ERROR, "Error opening snapshot file: %s\n", filename);
        return 1;
    }
    libspectrum_snap* snap = libspectrum_snap_alloc();
    if (snap == NULL) {
        msg_printf(MSG_ERROR, "Error loading snapshot file: %s\n", filename);
        return 1;
    }
    int error = 1;
    if ((buffer = malloc(s.st_size)) != NULL) {
        if ((error = file_load_blk(f, buffer, s.st_size)) == 0) {
            if ((error = libspectrum_snap_read(snap, buffer, s.st_size, LIBSPECTRUM_ID_UNKNOWN, filename)) == LIBSPECTRUM_ERROR_NONE)
                error = file_snap_cpy(snap, e, ram, r373, rest);
        }
        free(buffer);
    }
    libspectrum_snap_free(snap);
    fclose(f);
    if (error != 0) {
        msg_printf(MSG_WARN, "Error loading snapshot file: %s\n", filename);
    }
    return error;
}

int file_snap_save(const char* filename, const struct elwro* e) {
    const struct cpu* z = e->cpu;
    libspectrum_snap* snap = libspectrum_snap_alloc();
    if (snap == NULL) {
        msg_printf(MSG_ERROR, "Error saving snapshot file: %s\n", filename);
        return 1;
    }
    libspectrum_snap_set_machine(snap, LIBSPECTRUM_MACHINE_48);

    /* rejestry aktywne (bank 0 lub 1) idą jako główne, nieaktywne jako zapasowe */
    const int rg = z->alt_rg != 0, af = z->alt_af != 0;
    libspectrum_snap_set_a(snap, af ? z->As1 : z->As0);
    libspectrum_snap_set_f(snap, af ? z->Fs1 : z->Fs0);
    libspectrum_snap_set_a_(snap, af ? z->As0 : z->As1);
    libspectrum_snap_set_f_(snap, af ? z->Fs0 : z->Fs1);
    libspectrum_snap_set_bc(snap, rg ? z->BCs1 : z->BCs0);
    libspectrum_snap_set_bc_(snap, rg ? z->BCs0 : z->BCs1);
    libspectrum_snap_set_de(snap, rg ? z->DEs1 : z->DEs0);
    libspectrum_snap_set_de_(snap, rg ? z->DEs0 : z->DEs1);
    libspectrum_snap_set_hl(snap, rg ? z->HLs1 : z->HLs0);
    libspectrum_snap_set_hl_(snap, rg ? z->HLs0 : z->HLs1);
    libspectrum_snap_set_ix(snap, z->IXs);
    libspectrum_snap_set_iy(snap, z->IYs);
    libspectrum_snap_set_i(snap, z->Is);
    libspectrum_snap_set_r(snap, z->Rs);
    libspectrum_snap_set_sp(snap, z->SPs);
    libspectrum_snap_set_pc(snap, z->PCs);
    libspectrum_snap_set_iff1(snap, z->iff1);
    libspectrum_snap_set_iff2(snap, z->iff2);
    libspectrum_snap_set_im(snap, z->imode);
    libspectrum_snap_set_halted(snap, z->halt != 0);
    libspectrum_snap_set_out_ula(snap, e->scn->border_color & 0x07);

    /* strony 5, 2, 0 = 0x4000, 0x8000, 0xc000 (jak w file_snap_cpy) */
    static const struct { int page; int addr; } map[] = {{5, 0x4000}, {2, 0x8000}, {0, 0xc000}};
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        libspectrum_byte* page = libspectrum_new(libspectrum_byte, 0x4000);
        memcpy(page, e->ram + map[i].addr, 0x4000);
        libspectrum_snap_set_pages(snap, map[i].page, page);
    }

    libspectrum_byte* buffer = NULL;
    size_t length = 0;
    int flags = 0;
    int error = libspectrum_snap_write(&buffer, &length, &flags, snap, LIBSPECTRUM_ID_SNAPSHOT_SNA, NULL, 0);
    libspectrum_snap_free(snap);
    if (error == LIBSPECTRUM_ERROR_NONE) {
        FILE* f = fopen(filename, "wb");
        if (f == NULL || fwrite(buffer, length, 1, f) != 1) {
            msg_printf(MSG_ERROR, "Error writing snapshot file: %s\n", filename);
            error = 1;
        }
        if (f != NULL)
            fclose(f);
    } else {
        msg_printf(MSG_ERROR, "Error saving snapshot file: %s\n", filename);
    }
    libspectrum_free(buffer);
    if (error == 0) {
        msg_printf(MSG_MSG, "Snapshot saved: %s\n", filename);
        if (e->port_f7 != 0)
            msg_printf(MSG_WARN, "Port 0xF7 = 0x%02x (memory paging) is not stored in .sna\n", e->port_f7);
    }
    return error;
}

/****************************************************************/

void file_help(void) {
    printf(
        "e800j v0.2 - Elwro 800 Junior emulator - (C)2006 Krzysztof Komarnicki\n"
        "Usage:   e800j [options] [arguments]\n\n"
        "Arguments:\n"
        " <snapshot>          snapshot file loaded at startup (" HK_ALT_NAME "+F12 reloads it)\n\n"
        "Command line options:\n"
        " -h, --help                   this text\n"
        " -c, --config <fname>          alternative configuration file\n"
        " -p, --profile <pname>         configuration profile name (overrides main::profile)\n"
        " -f, --full-screen             fullscreen\n"
        " -n, --no-frame                no frame decoration\n"
        "     --bits-per-pixel <value>  color depth\n"
        " -x <value>                    X resolution\n"
        " -y <value>                    Y resolution\n"
        "     --debug                   debugger enable, emulator starts with debugger console\n"
        "                                (Windows: the debugger gets its own console window)\n"
        " -a, --a-read-only <fname>     mount CPCEMU *.DSK image in Drive A read-only\n"
        " -A, --a-read-write <fname>    mount CPCEMU *.DSK image in Drive A read-write (excludes -a), a missing file is created as a blank disk\n"
        " -b, --b-read-only <fname>     mount CPCEMU *.DSK image in Drive B read-only\n"
        " -B, --b-read-write <fname>    mount CPCEMU *.DSK image in Drive B read-write (excludes -b), a missing file is created as a blank disk\n"
        "     --log-level <value>       message level 0..6, default 0\n"
#ifdef _WIN32
        "\n"
        "On Windows '/' can be used instead of '-' and '--' (e.g. /h, /help, /config file).\n"
#endif
        "\n"
        "Hot keys (in the emulator window):\n"
        " F1                  show hot keys help overlay (Esc or any hot key closes)\n"
        " F5/F6               insert disk in drive A/B (file dialog, or next *.dsk from 'diskdir' in the profile)\n"
        " Shift+F5/F6         eject disk from drive A/B\n"
        " F2 / F3             volume down / up (progress bar shown at the bottom)\n"
        " F8                  screenshot e800j_NNN.png (352x288 with border)\n"
        " " HK_CTRL_NAME "+F8" HK_CTRL_PAD "              screenshot without border (256x192)\n"
        " F9                  fullscreen on / off\n"
        " Pause               pause / resume the emulation\n"
        " F11                 enter the Z80 debugger at any time (console; 'q' leaves it, 'g' resumes)\n"
        " " HK_ALT_NAME "+F5/F6" HK_ALT_PAD "           toggle RO/RW of disk in drive A/B\n"
        " " HK_WIN_NAME "+F5/F6" HK_WIN_PAD "           new blank (unformatted) disk in drive A/B: save dialog asks for the .dsk name and place\n"
        " " HK_CTRL_NAME "+F1..F5" HK_CTRL_PAD "         window scale x1 .. x3, step 0.5 (F1 = x1, F5 = x3)\n"
        " F12                 save snapshot e800j_NNN.sna\n"
        " Shift+F12           soft reset (CPU, memory mapping, FDC; RAM is kept)\n"
        " " HK_CTRL_NAME "+F12" HK_CTRL_PAD "            cold restart (as a fresh start: RAM randomized, then reset)\n"
        " " HK_ALT_NAME "+F12" HK_ALT_PAD "             cold restart, then reload the snapshot given on the command line\n"
        " " HK_WIN_NAME "+F12" HK_WIN_PAD "            cold restart, then load a snapshot chosen in a file dialog (cancel: restart only)\n"
        "\n"
        "Elwro dedicated keys (in the emulator window):\n"
        " Home                CP/J\n"
        " F10                 DIR\n"
        " End                 ERASE\n"
        " Page Up             LOAD\n"
        " Page Down           SAVE\n"
        //    " -trap_in_e <value>    trap on IN value enable\n"
        //    " -trap_in_d <value>    trap on IN value disable\n"
        //    " -trap_out_e <value>   trap on OUT value enable\n"
        //    " -trap_out_d <value>   trap on OUT value disable\n"
        //    " -trap_reg_e <r> <val> trap on reg value enable\n"
        //    " -trap_reg_d <r> <val> trap on reg value disable\n"
        //    " -lines <val>          line debuuger count\n"
        //    " -nojump		    instructions doesn't modify PC\n"
        "\n"
    );
    exit(0);
}
