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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libspectrum.h>
#include "e800j_file.h"
#include "e800j.h"
#include "z80_cpu.h"
#include "e800j_msg.h"

int file_lookup_option(int argc, char** argv, const char* name) {
    int i;
    for (i = 0; i < argc; i++)
        if (!strcmp(argv[i], name))
            return i;
    return 0;
}

char file_snap_cpy(libspectrum_snap* snap, z80* z, char* ram, char* reg373, char rest) {
    int i;

    if (!snap || !z || !ram || !reg373)
        return 1;

    libspectrum_machine machine;

    machine = libspectrum_snap_machine(snap);

    if ((machine != LIBSPECTRUM_MACHINE_48) && (machine != LIBSPECTRUM_MACHINE_16)) {
        msg_printf(MSG_WARN, "Nie obsługiwany typ komputera: %s\n", libspectrum_machine_name(machine));
        if (rest)
            return 0;
    }

    /* ustawienie rejestrów */
    z80_reset(z);

    z->As0 = libspectrum_snap_a(snap);
    z->Fs0 = libspectrum_snap_f(snap);
    z->As1 = libspectrum_snap_a_(snap);
    z->Fs1 = libspectrum_snap_f_(snap);

    z->BCs0 = libspectrum_snap_bc(snap);
    z->BCs1 = libspectrum_snap_bc_(snap);
    z->DEs0 = libspectrum_snap_de(snap);
    z->DEs1 = libspectrum_snap_de_(snap);
    z->HLs0 = libspectrum_snap_hl(snap);
    z->HLs1 = libspectrum_snap_hl_(snap);

    z->IXs = libspectrum_snap_ix(snap);
    z->IYs = libspectrum_snap_iy(snap);
    z->Is = libspectrum_snap_i(snap);
    z->Rs = libspectrum_snap_r(snap);
    z->SPs = libspectrum_snap_sp(snap);
    z->PCs = libspectrum_snap_pc(snap);

    z->iff1 = libspectrum_snap_iff1(snap);
    z->iff2 = libspectrum_snap_iff2(snap);
    z->imode = libspectrum_snap_im(snap);
    z->halt = libspectrum_snap_halted(snap);

    /* kolor ramki */
    *reg373 = libspectrum_snap_out_ula(snap);

    /* kopiowanie pamięci */
    for (i = 0; i < 16; i++)
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
            }
        }
    return 0;
}

char file_load_blk(FILE* f, unsigned char* buffer, long length) {
    return fread(buffer, length, 1, f) != 1;
}

int file_snap_load(const char* fname, z80* cp, char* ram, char* r373, char rest) {
    libspectrum_snap* snap = NULL;
    int error = 1;
    unsigned char* buffer;
    long length;
    FILE* f;

    if (!(f = fopen(fname, "r"))) {
#ifdef __cplusplus
        throw(string)"Error opening file " + fname;
#else
        msg_printf(MSG_WARN, "Error opening snapshot file  %s\n", fname);
        return -1;
#endif
    }

    if((snap=libspectrum_snap_alloc()) == NULL){
#ifdef __cplusplus
        throw(string)"Error loading snapshot " + fname;
#else
        msg_printf(MSG_WARN, "Error loading snapshot %s\n", fname);
        return error;
#endif
    }
#ifdef __cplusplus
    try{
#endif
    fseek(f, 0L,SEEK_END);
    length = ftell(f);
    fseek(f, 0L,SEEK_SET);
    error = 1;
    if ((buffer = (unsigned char*)malloc(length))) {
        if (!(error = file_load_blk(f, buffer, length))) {
            if (!(error = libspectrum_snap_read(snap, buffer, length, LIBSPECTRUM_ID_UNKNOWN, fname)))
                error = file_snap_cpy(snap, cp, ram, r373, rest);
        }
        free(buffer);
    }
    libspectrum_snap_free(snap);
    fclose(f);
#ifdef __cplusplus
    } catch(string a) {
        fclose(f);
        throw a;
    }
#endif

    if (error)
#ifdef __cplusplus
    throw(string)"Error loading snapshot " + fname;
#else
        msg_printf(MSG_WARN, "Error loading snapshot %s\n", fname);
#endif
    return error;
}

void file_get_cline(int argc, char** argv, z80* cp, char* ram, char* r373, char rest) {
    if (argc != 2)
        return;
    if (*argv[1] == '-')
        return;
    msg_printf(MSG_MSG, "Loading snapshot: %s\n", argv[1]);
    file_snap_load(argv[1], cp, ram, r373, rest);
}

/****************************************************************/

char file_is_letter(char a) {
    return (a >= '0' && a <= '9') || (a >= 'A' && a <= 'Z') ||
        (a >= 'a' && a <= 'z') || (a == '_') || (a == '-') || (a == '.');
}

char* file_find_cfg_name(const char* cfg, const char* name, int lvl, int* line) {
    int nest = 0, cmd = 0, prevch = 0, k, i, found = 0, chr = 0;
    char a;
    int z = strlen(cfg), u = strlen(name);
    for (i = 0; i < z; i++) {
        a = cfg[i];
        if (!chr) {
            if (a == '#')
                cmd++;
            if (a == '\n')
                cmd = 0, (*line)++;
            if (a == '\\')
                chr = 1;
        }
        else
            chr = 0;
        if (cmd)
            continue; /* jesli komentarz to pomiń następne sprawdzanie */
        if (found && file_is_letter(a))
            found = 0;
        if (found)
            return (char*)cfg + i;
        if (a == '{')
            nest++;
        if (a == '}')
            nest--;
        if (nest == lvl) {
            /* szukamy na właściwym poziomie zagnieżdzenia */
            if (file_is_letter(a) && !file_is_letter(prevch)) {
                for (k = 0; k < u && i < z && file_is_letter(cfg[i]) && cfg[i] == name[k]; k++, i++);
                i--;
                if (k == u)
                    found = 1;
            }
            prevch = a;
        }
    }
    return NULL;
}

char* file_find_cfg_option(const char* cfg, const char* name, const char* str, const char* err, int lvl, int xx) {
    char *tmp, eq_ = 0, *ptr = NULL, a, qt = 0, bs = 0, tg = 0;
    int line = 1, i, len = 0;
    int z = strlen(cfg);
    /**/
    tmp = file_find_cfg_name(cfg, name, lvl, &line);
    if (!tmp) {
        if (!xx)
            msg_printf(MSG_ERROR, "Missing option %s\n", name);
        return NULL;
    }
    for (i = 0; i < z; i++) {
        a = tmp[i];
        if (!bs) {
            if (a == '"')
                qt = !qt;
            if (a == '\\') {
                bs = 1;
                continue;
            }
            if (a == '=') {
                eq_ = 1;
                continue;
            }
        }
        else
            bs = 0;
        if ((!eq_ || tg) && file_is_letter(a)) {
            msg_printf(MSG_ERROR, "Config file syntax error in line %i\n", line);
            return NULL;
        }
        if (eq_) {
            if (((a > ' ') && (a <= 'z')) || ((a == ' ') && (bs || qt))) {
                if (len == 0)
                    ptr = tmp + i;
                len++;
            }
            if (a == ';') {
                if (str) {
                    strncpy((char*)str, ptr, len);
                    ((char*)str)[len - 1] = 0;
                }
                else
                    return (void*)((long)line);
                return (char*)ptr + len;
            }
            if (!file_is_letter(a) && !qt && !bs && len > 0)
                tg = 1;
        }
    }
    return NULL;
}

char* file_find_cfg_section(const char* cfg, const char* name, const char* str, const char* err, int lvl) {
    char* tmp;
    int line = 1, i, nest = 0;
    int z;

    tmp = file_find_cfg_name(cfg, "section", lvl, &line);
    if (!tmp) {
        msg_printf(MSG_ERROR, "Config file error: There are no sections in the config file.\n");
        return NULL;
    }
    tmp = file_find_cfg_name(tmp, name, lvl, &line);
    if (!tmp) {
        msg_printf(MSG_ERROR, "Config file error: missing section %s\n", name);
        return NULL;
    }
    z = strlen(tmp);

    for (i = 0; i < z && *tmp != '{'; i++, tmp++)
        if (*tmp > ' ') {
            i = z;
            break;
        }
    if (i == z) {
        if (*tmp > ' ')
            msg_printf(MSG_ERROR, "Config file error: syntax error at section %s near line %i\n", name, line);
        else
            msg_printf(MSG_ERROR, "Config file error: missing '{' to open section %s near line %i\n", name, line);
        return NULL;
    }
    else
        tmp++;

    for (i = 0; i < z && (!(tmp[i] == '}' && !nest)); i++) {
        if (tmp[i] == '{')
            nest++;
        if (tmp[i] == '}')
            nest--;
    }

    if (i == z) {
        msg_printf(MSG_ERROR, "Config file error: missing '}' to close section %s\n", name);
        return NULL;
    }
    if (str) {
        strncpy((char*)str, tmp, i);
        ((char*)str)[i] = 0;
        return (char*)str;
    };
    return NULL;
}

/**************************************************************/

char* file_get_profile(const char* cfg, char* pname) {
    char* tmp;
    if (!(tmp = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return NULL;
    }
    msg_printf(MSG_MSG, "Loading ROM BIOS\n");
    if (!file_find_cfg_section(cfg, "main", tmp,NULL, 0)) {
        free(tmp);
        return NULL;
    }
    if (!file_find_cfg_option(cfg, "profile", tmp,NULL, 1, 0)) {
        free(tmp);
        return NULL;
    }
    msg_printf(MSG_MSG, "Choose profile: %s\n", tmp);
    if (!file_find_cfg_section(cfg, tmp, pname,NULL, 0)) {
        free(tmp);
        return NULL;
    }
    free(tmp);
    return pname;
}

void file_unquote(char* str) {
    int k = strlen(str);

    if (str[0] == '"')
        strcpy(str, str + 1);
    for (; k; k--) {
        if (file_is_letter(k))
            break;
        if (str[k] == '"') {
            str[k] = 0;
            break;
        }
    }
}

char file_read_rom(char* fn, int addr, char* rom, int romsize) {
    FILE* f;
    int len;

    if (!(f = fopen(fn, "r"))) {
        msg_printf(MSG_ERROR, "\nError opening file %s\n", fn);
        return 1;
    }

    fseek(f, 0L,SEEK_END);
    len = ftell(f);
    fseek(f, 0L,SEEK_SET);
    if (addr + len > romsize) {
        msg_printf(MSG_ERROR, "\nError file %s is to large for max rom size "
                   "0x0%x at specified address 0x0%x\n", fn, romsize, addr);
        return 1;
    }

    if (fread(rom + addr, len, 1, f) != 1) {
        msg_printf(MSG_ERROR, "\nError loading file %s (filesystem error ?)\n", fn);
        return 1;
    }

    fclose(f);
    return 0;
}

int file_bios_load(const char* cfg, char* rom, char* profile, int romsize) {
    char *tmp, *tmp1, *path, *fn, *x = NULL;
    int addr;
    char ttt[256];
    if (!(tmp = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (!(tmp1 = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        free(tmp);
        return 1;
    }
    if (!file_find_cfg_option(profile, "rombios", tmp,NULL, 0, 0)) {
        free(tmp);
        return 1;
    }
    msg_printf(MSG_MSG, "ROMBIOS section: %s\n", tmp);
    if (!file_find_cfg_section(cfg, tmp, tmp1,NULL, 0)) {
        free(tmp);
        free(tmp1);
        return 1;
    }
    if (!file_find_cfg_option(tmp1, "set_bios", tmp,NULL, 0, 0)) {
        free(tmp);
        return 1;
    }
    strcpy(ttt, tmp);
    msg_printf(MSG_MSG, "ROM choose: %s\n", ttt);
    if (!file_find_cfg_option(tmp1, "rompath", tmp,NULL, 0, 0)) {
        free(tmp);
        return 1;
    }
    if (!(path = (char*)malloc(strlen(tmp) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        free(tmp1);
        free(tmp);
        return 1;
    }
    strcpy(path, tmp);
    file_unquote(path);
    msg_printf(MSG_MSG, "Path to ROM files: %s\n", path);
    if (!file_find_cfg_section(tmp1, ttt, tmp,NULL, 0)) {
        free(tmp);
        free(tmp1);
        free(path);
        return 1;
    }
    for (x = tmp; x;) {
        if (!file_find_cfg_option(x, "address", ttt,NULL, 0, 1))
            break;
        if (!(x = file_find_cfg_option(x, "file", tmp1,NULL, 0, 0))) {
            msg_printf(MSG_ERROR, "Error: brak nazwy pliku.\n");
            free(tmp1);
            free(tmp);
        }
        file_unquote(tmp1);
        file_unquote(ttt);
        if (!(fn = (char*)malloc(strlen(tmp1) + strlen(ttt) + 1))) {
            msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
            free(tmp1);
            free(tmp);
            free(path);
            return 1;
        }
        sprintf(fn, "%s%s", path, tmp1);

        addr = strtol(ttt,NULL, 16);
        msg_printf(MSG_MSG, "Loading file %s to ROM banks at address 0x0%x .... ", fn, addr);
        if (file_read_rom(fn, addr, rom, romsize)) {
            free(fn);
            free(tmp1);
            free(tmp);
            free(path);
            return 1;
        }
        msg_printf(MSG_MSG, "done.\n");
        free(fn);
    }
    free(path);
    free(tmp1);
    free(tmp);
    return 0;
}

void file_free_buffers(int n, void* el) {
    //    free(((elwro*)el)->rom);
    //    free(((elwro*)el)->ram);
}

/*******************************************/
char file_memo_set(char* cfg, elwro* el, char* profile) {
    char tmp0[512], *tmp1;
    int i;

    if (!(tmp1 = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (!file_find_cfg_option(profile, "memory", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "Memory section is %s\n", tmp0);
    if (!file_find_cfg_section(cfg, tmp0, tmp1,NULL, 0)) {
        free(tmp1);
        return 1;
    }

    if (!file_find_cfg_option(tmp1, "rom_bank_size", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "ROM buffer set to %s byte size.\n", tmp0);
    el->rom_size = strtol(tmp0,NULL, 16);
    if (el->rom_size < 8192)
        msg_printf(MSG_WARN, "Very small ROM buffer size. Is it a correct size ?\n");
    if (!(el->rom = (char*)malloc(el->rom_size))) {
        msg_printf(MSG_ERROR, "Error allocate memory for ROM buffer.\n");
        free(tmp1);
        return 1;
    }
    for (i = 0; i < el->rom_size; i++)
        el->rom[i] = rand();
    if (!file_find_cfg_option(tmp1, "ram_bank_size", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "RAM buffer set to %s byte size.\n", tmp0);
    el->ram_size = strtol(tmp0,NULL, 16);
    if (el->ram_size < 8192)
        msg_printf(MSG_WARN, "Very small RAM buffer size. Is it a correct size ?\n");
    if (!(el->ram = (char*)malloc(el->ram_size))) {
        msg_printf(MSG_ERROR, "Error allocate memory for RAM buffer.\n");
        free(el->rom);
        free(tmp1);
        return 1;
    }
    for (i = 0; i < el->ram_size; i++)
        el->ram[i] = rand();
    on_exit(file_free_buffers, el);
    free(tmp1);
    return 0;
}

/*******************************************/
char file_set_cpu(char* cfg, elwro* el, char* profile) {
    char tmp0[512], *tmp1;
    if (!file_find_cfg_option(profile, "cpu", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "CPU section is %s\n", tmp0);
    if (!(tmp1 = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (!file_find_cfg_section(cfg, tmp0, tmp1,NULL, 0)) {
        free(tmp1);
        return 1;
    }
    if (!file_find_cfg_option(tmp1, "call_fq", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->cpu_rfsh = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set CPU call to: %i [ms]\n", el->cpu_rfsh);
    if (!file_find_cfg_option(tmp1, "atom", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->tr = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set %i tics/call\n", el->tr);
    if (!file_find_cfg_option(tmp1, "intfq", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->ifreq = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set IRQ call for %i tics of CPU\n", el->ifreq);
    free(tmp1);
    return 0;
}

/*******************************************/
char file_set_screen(char* cfg, elwro* el, char* profile) {
    char tmp0[1024], *tmp1;
    if (!file_find_cfg_option(profile, "screen", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "Screen section is %s\n", tmp0);
    if (!(tmp1 = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (!file_find_cfg_section(cfg, tmp0, tmp1,NULL, 0)) {
        free(tmp1);
        return 1;
    }
    if (!file_find_cfg_option(tmp1, "blink", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->flash = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set blinking to: %i [ms]\n", el->zx->flash);
    if (!file_find_cfg_option(tmp1, "rfsh", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->rfsh = strtol(tmp0,NULL, 0);
    if (!file_find_cfg_option(tmp1, "brdsize", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->bsize = strtod(tmp0,NULL);
    msg_printf(MSG_MSG, "Set border size to: %.3f of window size \n", el->zx->bsize);
    msg_printf(MSG_MSG, "Set refresh screen to %i [ms]\n", el->zx->rfsh);
    if (!file_find_cfg_section(tmp1, "palette", tmp0,NULL, 0)) {
        free(tmp1);
        return 1;
    }
    if (!file_find_cfg_option(tmp0, "bright", tmp1,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->bright = strtol(tmp1,NULL, 0);
    msg_printf(MSG_MSG, "Set palette bright to: %i \n", el->zx->bright);
    if (!file_find_cfg_option(tmp0, "brt_normal", tmp1,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->nbrt = strtol(tmp1,NULL, 0);
    msg_printf(MSG_MSG, "Set palette bright normal to: %i \n", el->zx->nbrt);
    if (!file_find_cfg_option(tmp0, "brt_brt", tmp1,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->bbrt = strtol(tmp1,NULL, 0);
    msg_printf(MSG_MSG, "Set palette bright 'bright' to: %i \n", el->zx->bbrt);
    if (!file_find_cfg_option(tmp0, "brt_wht", tmp1,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->wbrt = strtol(tmp1,NULL, 0);
    msg_printf(MSG_MSG, "Set palette bright white color to: %i \n", el->zx->wbrt);

    free(tmp1);
    return 0;
}

/*******************************************/
char file_set_video(char* cfg, elwro* el, char* profile) {
    char tmp0[512], *tmp1;
    if (!file_find_cfg_option(profile, "video", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "Video section is %s\n", tmp0);
    if (!(tmp1 = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (!file_find_cfg_section(cfg, tmp0, tmp1,NULL, 0)) {
        free(tmp1);
        return 1;
    }

    if (!file_find_cfg_option(tmp1, "width", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->width = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set width to : %i\n", el->zx->width);
    if (!file_find_cfg_option(tmp1, "height", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->height = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set height to: %i\n", el->zx->height);
    if (!file_find_cfg_option(tmp1, "bpp", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->bpp = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set bpp to: %i\n", el->zx->bpp);
    if (!file_find_cfg_option(tmp1, "fscrn", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->zx->fscrn = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Set fullscreen to: %i\n", el->zx->fscrn);

    free(tmp1);
    return 0;
}

/*******************************************/
char file_set_sound(char* cfg, elwro* el, char* profile) {
    char tmp0[512], *tmp1;
    if (!file_find_cfg_option(profile, "sound", tmp0,NULL, 0, 0))
        return 1;
    msg_printf(MSG_MSG, "Sound section is %s\n", tmp0);
    if (!(tmp1 = (char*)malloc(strlen(cfg) + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (!file_find_cfg_section(cfg, tmp0, tmp1,NULL, 0)) {
        free(tmp1);
        return 1;
    }
    if (!file_find_cfg_option(tmp1, "nosound", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->nosound = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Sound : %s\n", el->snd->nosound ? "off" : "on");
    if (el->snd->nosound) {
        free(tmp1);
        return 0;
    }

    if (!file_find_cfg_option(tmp1, "channels", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->chn = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Channels : %i\n", el->snd->chn);
    if (!file_find_cfg_option(tmp1, "rbuffer", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->size = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Sound round buffer size : %i\n", el->snd->size);
    if (!file_find_cfg_option(tmp1, "samplert", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->freq = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Sampling freq : %i Hz\n", el->snd->freq);
    if (!file_find_cfg_option(tmp1, "dbuffer", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->len = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Driver line buffer size: %i\n", el->snd->len);
    if (!file_find_cfg_option(tmp1, "mixvol", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->volume = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Mix volume: %i\n", el->snd->volume);
    if (!file_find_cfg_option(tmp1, "scale", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->scale = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Max samples / CPU atom: %i\n", el->snd->scale);

    if (!file_find_cfg_option(tmp1, "alias", tmp0,NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    el->snd->alias = strtol(tmp0,NULL, 0);
    msg_printf(MSG_MSG, "Wave antyalias: %s\n", el->snd->alias ? "on" : "off");

    free(tmp1);
    return 0;
}

/*******************************************/
char file_init(elwro* el, char* cpath, int argc, char** argv) {
    FILE* f;
    char *cfg, *profile;
    long cfglen;
    int i;

    if (file_lookup_option(argc, argv, "-debug"))
        el->debug = 1;

    if ((i = file_lookup_option(argc, argv, "-cfg"))) {
        if (i + 1 >= argc) {
            msg_printf(MSG_ERROR, "Missing argument for -cfg option.\n");
            return 1;
        }
        cpath = argv[i + 1];
    }

    if (!(f = fopen(cpath, "r"))) {
        msg_printf(MSG_ERROR, "Error opening config file: %s\n", cpath);
        return 1;
    }

    msg_printf(MSG_MSG, "Opened config file: %s\n", cpath);
    fseek(f, 0L,SEEK_END);
    cfglen = ftell(f);
    fseek(f, 0L,SEEK_SET);

    if (!(cfg = (char*)malloc(cfglen + 1))) {
        msg_printf(MSG_ERROR, "Error memory allocation for config file\n");
        return 1;
    }

    if (fread(cfg, cfglen, 1, f) != 1) {
        msg_printf(MSG_ERROR, "Error reading config file, possibly filesystem or disk problem.\n");
        return 1;
    }
    fclose(f);

    if (!(profile = (char*)malloc(cfglen + 1))) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        free(cfg);
        return 1;
    }

    if (!(file_get_profile(cfg, profile)))
        return 1;

    if (file_lookup_option(argc, argv, "-profile")) {
        //	strcpy(profile,argv[i+1]);
        //	msg_printf(MSG_MSG, "Forced profile by command line option to %s\n", profile);
        //        if(!file_find_cfg_section(cfg,tmp,profile,NULL,0)) { free(tmp); return NULL; }
    }

    if (file_memo_set(cfg, el, profile))
        return 1;
    if (file_bios_load(cfg, el->rom, profile, el->rom_size))
        return 1;
    if (file_set_cpu(cfg, el, profile))
        return 1;
    if (file_set_screen(cfg, el, profile))
        return 1;
    if (file_set_video(cfg, el, profile))
        return 1;
    if (file_set_sound(cfg, el, profile))
        return 1;
    /* dojdą dodatkowe*/
    free(profile);
    free(cfg);
    return 0;
}

void file_option_param(int argc, char** argv, int pos, int min, int max, int* val) {
    int tmp;
    if (pos + 1 >= argc) {
        msg_printf(MSG_WARN, "Missing parameter to option %s\n", argv[pos]);
        return;
    }
    tmp = strtol(argv[pos + 1],NULL, 0);
    if (tmp < min) {
        msg_printf(MSG_WARN, "Too small parameter for option %s=%i (min=%i)\n", argv[pos], tmp, min);
        return;
    }
    if (tmp > max) {
        msg_printf(MSG_WARN, "Too large parameter for option %s=%i (max=%i)\n", argv[pos], tmp, max);
        return;
    }
    *val = tmp;
}

void file_help() {
    printf(
        "e800j v0.2 - Elwro 800 Junior emulator - (C)2006 Krzysztof Komarnicki\n"
        "Usage:   e800j [snapshot filename] [options]\n\n"
        " -h,-help,--help     this text\n"
        " -cfg <fname>        alternative configiguration file\n"
        " -profile <pname>    configuration profile name\n"
        " -fs                 fullscreen\n"
        " -nf                 no frame decoration\n"
        " -bpp <value>        color deep\n"
        " -xres <value>       X resolution\n"
        " -yres <value>       Y resolution\n"
        " -debug              debugger enable, emulator starts with debugger console\n"
        " -adisk <fname>      disk image in CPCEMU *.DSK format for Drive A\n"
        " -bdisk <fname>      disk image in CPCEMU *.DSK format for Drive B\n"
        " -arw                disk image A open in R/W mode\n"
        " -brw                disk image B open in R/W mode\n"
        " -msg_lvl <value>    message level 0..6, default 0\n"
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
