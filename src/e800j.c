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

#include <SDL3/SDL_main.h>
#ifdef __APPLE__
#include <string.h>
#include <unistd.h>
#include <SDL3/SDL_filesystem.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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
#include "e800j_cmdline.h"
#include "e800j_disk.h"
#include "virt_fdd.h"
#include "e800j_msg.h"

static struct screen screen;

static void disk_open(const struct cmdline* cl, const int n, struct vfdd_disk* disk) {
    const char* path = cl->disk[n];
    if (path == nullptr)
        return;
    const char dn = (char)('A' + n);
    const bool rw = cl->disk_rw[n];
    if (vfdd_dsk_floppy(disk, VFDD_LOAD, rw ? VFDD_RW : VFDD_RO, 0, 0, path) < 0) {
        msg_printf(MSG_ERROR, "Opening file %s failed.\n", path);
    }
    msg_printf(MSG_MSG, "Open file %s for %s as drive %c.\n", path, rw ? "RW" : "RO", dn);
}

/* Opcje linii poleceń nadpisują wartości wczytane z pliku konfiguracji (wywoływane po file_init). */
static void cmdline_override(struct elwro* e, const struct cmdline* cmd) {
    if (cmd->snapshot != nullptr)
        snprintf(e->snap_path, sizeof(e->snap_path), "%s", cmd->snapshot);
    if (cmd->debug)
        e->debug = 1;
    if (cmd->bpp != CMDLINE_UNSET)
        e->scn->bpp = cmd->bpp;
    if (cmd->xres != CMDLINE_UNSET)
        e->scn->width = cmd->xres;
    if (cmd->yres != CMDLINE_UNSET)
        e->scn->height = cmd->yres;
    if (cmd->fullscreen)
        e->scn->fscrn = 1;
    if (cmd->noframe)
        e->scn->noframe = 1;
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

int elwro_restart_snap(struct elwro* e, const char* path) {
    elwro_cold_reset(e);
    if (path == nullptr || path[0] == 0)
        return 1;
    if (file_snap_load(path, e, e->ram, &e->scn->border_color, 0) != 0)
        return 1;
    return 0;
}

static const char* snap_basename(const char* path) {
    const char* base = path;
    for (const char* p = path; *p != 0; ++p) {
        if (*p == '/' || *p == '\\')
            base = p + 1;
    }
    return base;
}

void elwro_snap_announce(struct elwro* e, const char* path, const int error) {
    char text[320];
    if (path == nullptr || path[0] == 0) {
        msg_printf(MSG_MSG, "No snapshot loaded\n");
        snprintf(text, sizeof(text), "No snapshot loaded");
    } else {
        snprintf(text, sizeof(text), error != 0 ? "Snapshot load failed: %s" : "Snapshot loaded: %s", snap_basename(path));
    }
    screen_toast_next(e->scn, text);
}

int elwro_snap_restart(struct elwro* e) {
    return elwro_restart_snap(e, e->snap_path);
}

int main(const int argc, char** args) {
    struct elwro e;
    struct cmdline cmd;
    struct fdc fdc;
    struct cpu cpu;
    struct dbg dbg;
    struct sound snd;
    struct keyboard kbd;
    struct vfdd_disk disk_a, disk_b;

#ifdef _WIN32
    /* aplikacja GUI: podlacz sie do konsoli procesu nadrzednego (jesli jest) dla --help/--debug */
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        (void)freopen("CONOUT$", "w", stdout);
        (void)freopen("CONOUT$", "w", stderr);
        (void)freopen("CONIN$", "r", stdin);
    }
#endif
#ifdef __APPLE__
    /* uruchomienie z elwro.app (Finder): katalog biezacy to "/", zasoby sa w Contents/Resources */
    {
        char cwd[8];
        const char* base = SDL_GetBasePath();
        if (base != nullptr && getcwd(cwd, sizeof(cwd)) != nullptr && strcmp(cwd, "/") == 0)
            (void)chdir(base);
    }
#endif

    e.cpu = &cpu;
    e.scn = &screen;
    e.snd = &snd;
    e.kbd = &kbd;
    e.fdc = &fdc;
    e.disk[0] = &disk_a;
    e.disk[1] = &disk_b;
    e.diskdir[0] = 0;
    e.snap_path[0] = 0;
    screen.toast_ms = 10000;
    e.mem_mask = 0;
    e.map_mask = -1;
    e.junet_addr = 0xf3;
    e.debug = 0;

    /* linia poleceń jest interpretowana jednorazowo, dalej używana jest tylko struct cmdline */
    if (!cmdline_parse(&cmd, argc, (const char**)args)) {
        return 1;
    }
    if (cmd.help) {
        file_help();
    }
    if (cmd.msg_level != CMDLINE_UNSET) {
        msg_level = cmd.msg_level;
    }
    vfdd_dsk_floppy(&disk_a, VFDD_INIT, 0, 0, 0, nullptr);
    vfdd_dsk_floppy(&disk_b, VFDD_INIT, 0, 0, 0, nullptr);

    /* najpierw odczyt konfiguracji i na tej podstawie ustawienie e800, potem nadpisanie jej opcjami z linii poleceń */

    if (file_init(&e, cmd.cfg != nullptr ? cmd.cfg : "e800j.cfg", cmd.profile) == false) {
        return -1;
    }
    cmdline_override(&e, &cmd);

    disk_open(&cmd, 0, &disk_a);
    disk_open(&cmd, 1, &disk_b);

    screen.zx_patt = e.ram + 16384;
    screen.zx_attr = e.ram + 16384 + 6 * 1024;
    screen_border(&screen, 1);

    key_init(&kbd);
    z80_init(&cpu, mem_e800j_read, mem_e800j_write, io_read, io_write, &e, nullptr);
    dbg_init(&dbg, &cpu, e.debug, 1);

    fdc_init(&fdc);
    i8272_fdd_connect(&fdc.i8272, vfdd_drive, &disk_a, &disk_b, nullptr, nullptr);

    if (screen_init(&screen, e.scn->rfsh) == false) {
        return -1;
    }

    snd_init(&snd, &cpu, (double)e.tr * 1000.0 / (double)e.cpu_refresh);
    int snap_error = 0;
    if (e.snap_path[0] != 0) {
        msg_printf(MSG_MSG, "Loading snapshot: %s\n", e.snap_path);
        snap_error = file_snap_load(e.snap_path, &e, e.ram, &e.scn->border_color, 0);
    }

    disk_announce(&e);
    /* komunikat o snapshocie po zniknięciu komunikatu o dyskietkach; bez snapshotu w linii poleceń nic nie pokazujemy */
    if (e.snap_path[0] != 0)
        elwro_snap_announce(&e, e.snap_path, snap_error);
    zx_main(&e);

    snd_free_buffers(&snd);
    vfdd_dsk_floppy(&disk_a, VFDD_UNLOAD, 0, 0, 0, nullptr);
    vfdd_dsk_floppy(&disk_b, VFDD_UNLOAD, 0, 0, 0, nullptr);
    return 0;
}
