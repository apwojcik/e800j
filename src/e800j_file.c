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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libspectrum.h>
#include "e800j_file.h"

#include <sys/stat.h>

#ifdef _WIN32
#define stat _stat
#endif

#include "e800j.h"
#include "e800j_compat.h"
#include "z80_cpu.h"
#include "e800j_msg.h"
#include "e800j_sound.h"
#include "zx_scrn.h"

/* strtol() zawężone do zakresu int */
static int file_cfg_int(const char* str, int base) {
    long val = strtol(str, NULL, base);
    if (val > INT_MAX)
        val = INT_MAX;
    else if (val < INT_MIN)
        val = INT_MIN;
    return (int)val;
}

int file_lookup_option(const int argc, const char** argv, const char* name) {
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], name) == 0)
            return i;
    return 0;
}

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
    z80_reset(e->cpu);

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
    if ((f = fopen(filename, "rb")) == NULL) {
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

void file_get_cline(const int argc, const char** argv, const struct elwro* e, unsigned char* ram, unsigned char* r373, const char rest) {
    if (argc == 2 && *argv[1] != '-') {
        msg_printf(MSG_MSG, "Loading snapshot: %s\n", argv[1]);
        file_snap_load(argv[1], e, ram, r373, rest);
    }
}

/****************************************************************/

static int is_letter(const int a) {
    return (a >= '0' && a <= '9') || (a >= 'A' && a <= 'Z') ||
        (a >= 'a' && a <= 'z') || (a == '_') || (a == '-') || (a == '.');
}

static char* file_find_cfg_name(const char* cfg, const char* name, int lvl, int* line) {
    int nest = 0, cmd = 0, prevch = 0, found = 0, chr = 0;
    size_t k;
    const size_t z = strlen(cfg);
    const size_t u = strlen(name);
    for (size_t i = 0; i < z; i++) {
        const int a = (int)cfg[i];
        if (chr == 0) {
            if (a == '#')
                cmd++;
            if (a == '\n')
                cmd = 0, (*line)++;
            if (a == '\\')
                chr = 1;
        } else {
            chr = 0;
        }
        if (cmd)
            continue; /* jesli komentarz to pomiń następne sprawdzanie */
        if (found && is_letter(a))
            found = 0;
        if (found)
            return (char*)cfg + i;
        if (a == '{')
            nest++;
        if (a == '}')
            nest--;
        if (nest == lvl) {
            /* szukamy na właściwym poziomie zagnieżdżenia */
            if (is_letter(a) && !is_letter(prevch)) {
                for (k = 0; k < u && i < z && is_letter(cfg[i]) && cfg[i] == name[k]; k++, i++) {}
                --i;
                if (k == u)
                    found = 1;
            }
            prevch = a;
        }
    }
    return NULL;
}

char* file_find_cfg_option(const char* cfg, const char* name, char* str, const char* err, const int lvl, const int xx) {
    (void)err;
    char eq_ = 0, *ptr = NULL, qt = 0, bs = 0, tg = 0;
    int line = 1, length = 0;
    const size_t z = strlen(cfg);
    /**/
    char* tmp = file_find_cfg_name(cfg, name, lvl, &line);
    if (tmp == NULL) {
        if (xx == 0) {
            msg_printf(MSG_ERROR, "Missing option %s\n", name);
        }
        return NULL;
    }
    for (size_t i = 0; i < z; i++) {
        const char a = tmp[i];
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
        } else {
            bs = 0;
        }
        if ((!eq_ || tg) && is_letter(a)) {
            msg_printf(MSG_ERROR, "Config file syntax error in line %i\n", line);
            return NULL;
        }
        if (eq_) {
            if ((a > ' ' && a <= 'z') || (a == ' ' && (bs || qt))) {
                if (length == 0) {
                    ptr = tmp + i;
                }
                ++length;
            }
            if (a == ';') {
                if (str) {
                    strncpy(str, ptr, length);
                    str[length - 1] = 0;
                } else {
                    return (void*)(intptr_t)line;
                }
                return ptr + length;
            }
            if (is_letter(a) == 0 && !qt && !bs && length > 0) {
                tg = 1;
            }
        }
    }
    return NULL;
}

char* file_find_cfg_section(const char* cfg, const char* name, char* str,
    const char* err, const int lvl)
{
    (void)err;
    int line = 1, nest = 0;
    size_t i;

    char* tmp = file_find_cfg_name(cfg, "section", lvl, &line);
    if (tmp == NULL) {
        msg_printf(MSG_ERROR, "Config file error: There are no sections in the config file.\n");
        return NULL;
    }
    tmp = file_find_cfg_name(tmp, name, lvl, &line);
    if (tmp == NULL) {
        msg_printf(MSG_ERROR, "Config file error: missing section %s\n", name);
        return NULL;
    }
    const size_t z = strlen(tmp);

    for (i = 0; i < z && *tmp != '{'; i++, tmp++) {
        if (*tmp > ' ') {
            i = z;
            break;
        }
    }
    if (i == z) {
        if (*tmp > ' ') {
            msg_printf(MSG_ERROR, "Config file error: syntax error at section %s near line %i\n", name, line);
        } else {
            msg_printf(MSG_ERROR, "Config file error: missing '{' to open section %s near line %i\n", name, line);
        }
        return NULL;
    }
    ++tmp;

    for (i = 0; i < z && (!(tmp[i] == '}' && !nest)); i++) {
        if (tmp[i] == '{') {
            ++nest;
        }
        if (tmp[i] == '}') {
            --nest;
        }
    }
    if (i == z) {
        msg_printf(MSG_ERROR, "Config file error: missing '}' to close section %s\n", name);
        return NULL;
    }
    if (str != NULL) {
        strncpy((char*)str, tmp, i);
        ((char*)str)[i] = 0;
        return (char*)str;
    };
    return NULL;
}

/**************************************************************/

char* file_get_profile(const char* cfg, char* profile) {
    char* tmp;
    if ((tmp = malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return NULL;
    }
    if (file_find_cfg_section(cfg, "main", tmp, NULL, 0) == NULL) {
        free(tmp);
        return NULL;
    }
    if (file_find_cfg_option(cfg, "profile", tmp, NULL, 1, 0) == NULL) {
        free(tmp);
        return NULL;
    }
    msg_printf(MSG_MSG, "Chosen profile: %s\n", tmp);
    if (file_find_cfg_section(cfg, tmp, profile, NULL, 0) == NULL) {
        free(tmp);
        return NULL;
    }
    free(tmp);
    return profile;
}

static char* file_unquote(const char* s) {
    const char* p = s;
    if (*p == '"') {
        ++p;
    }
    const char* e = p;
    for (; *e != 0 && *e != '"'; ++e) {
    }
    const size_t length = e - p;
    char* result = malloc(length + 1);
    if (result == NULL) {
        msg_printf(MSG_ERROR, "Error allocating memory for an unquoted string.\n");
        return NULL;
    }
    result[length] = 0;
    memcpy(result, p, length);
    return result;
}

static int file_read_rom(char* filename, const int addr, const unsigned char* rom, int rom_size) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        FILE* f;
        if ((f = fopen(filename, "rb")) == NULL) {
            msg_printf(MSG_ERROR, "\nError opening file %s\n", filename);
            return 1;
        }
        if (addr + st.st_size > rom_size) {
            msg_printf(MSG_ERROR, "\nError file %s is to large for max rom size "
                       "0x0%x at specified address 0x0%x\n", filename, rom_size, addr);
            fclose(f);
            return 1;
        }
        const size_t size = st.st_size;
        if (fread((void*)(rom + addr), 1, size, f) < size) {
            if (feof(f) != 0) {
                msg_printf(MSG_ERROR, "Unexpected EOF while reading ROM file: %s\n", filename);
                fclose(f);
                return 1;
            }
            if (ferror(f) != 0) {
                msg_printf(MSG_ERROR, "Error reading ROM file: %s\n", filename);
                fclose(f);
                return 1;
            }
        }
        fclose(f);
        return 0;
    }
    msg_printf(MSG_ERROR, "File does not exists: %s\n", filename);
    return 1;
}

int file_bios_load(const char* cfg, const unsigned char* rom, const char* profile, const int rom_size) {
    char *tmp, *tmp1;
    char ttt[256];
    if ((tmp = malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if ((tmp1 = malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        free(tmp);
        return 1;
    }
    if (file_find_cfg_option(profile, "rombios", tmp, NULL, 0, 0) == NULL) {
        free(tmp);
        free(tmp1);
        return 1;
    }
    msg_printf(MSG_MSG, "ROMBIOS section: %s\n", tmp);
    if (file_find_cfg_section(cfg, tmp, tmp1, NULL, 0) == NULL) {
        free(tmp);
        free(tmp1);
        return 1;
    }
    if (file_find_cfg_option(tmp1, "set_bios", tmp, NULL, 0, 0) == NULL) {
        free(tmp);
        free(tmp1);
        return 1;
    }
    strcpy(ttt, tmp);
    msg_printf(MSG_MSG, "ROM chosen: %s\n", ttt);
    if (file_find_cfg_section(tmp1, ttt, tmp, NULL, 0) == NULL) {
        free(tmp);
        free(tmp1);
        return 1;
    }
    const char* x = tmp;
    while (x != NULL) {
        if (file_find_cfg_option(x, "address", ttt, NULL, 0, 1) == NULL) {
            break;
        }
        char* tt = file_unquote(ttt);
        const int addr = strtol(tt, NULL, 16);
        free(tt);
        if ((x = file_find_cfg_option(x, "file", tmp1, NULL, 0, 0)) == NULL) {
            msg_printf(MSG_ERROR, "Error: brak nazwy pliku.\n");
            break;
        }
        char* t1 = file_unquote(tmp1);
        msg_printf(MSG_MSG, "Loading file %s to ROM banks at the address 0x%X...\n", t1, addr);
        if (file_read_rom(t1, addr, rom, rom_size)) {
            free(tmp);
            free(tmp1);
            free(t1);
            return 1;
        }
        free(t1);
    }
    free(tmp);
    free(tmp1);
    return 0;
}

static void file_free_buffers(int n, void* e) {
    (void)n;
    (void)e;
    //    free(((struct elwro*)e)->rom);
    //    free(((struct elwro*)e)->ram);
}

/*******************************************/
static char file_memo_set(const char* cfg, struct elwro* e, const char* profile) {
    char tmp0[512];

    char* tmp1 = (char*)malloc(strlen(cfg) + 1);
    if (tmp1 == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (file_find_cfg_option(profile, "memory", tmp0, NULL, 0, 0) == NULL) {
        return 1;
    }
    msg_printf(MSG_MSG, "Memory section is %s\n", tmp0);
    if (file_find_cfg_section(cfg, tmp0, tmp1, NULL, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    if (file_find_cfg_option(tmp1, "rom_bank_size", tmp0, NULL, 0, 0) == NULL) {
        return 1;
    }
    e->rom_size = strtol(tmp0, NULL, 16);
    msg_printf(MSG_MSG, "ROM buffer set to %u bytes.\n", e->rom_size);

    if (e->rom_size < 8192) {
        msg_printf(MSG_WARN, "Very small ROM buffer size. Is it a correct size ?\n");
    }
    if ((e->rom = malloc(e->rom_size)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for ROM buffer.\n");
        free(tmp1);
        return 1;
    }
    for (int i = 0; i < e->rom_size; i++) {
        e->rom[i] = (unsigned char)rand();
    }
    if (file_find_cfg_option(tmp1, "ram_bank_size", tmp0, NULL, 0, 0) == NULL) {
        return 1;
    }
    e->ram_size = strtol(tmp0, NULL, 16);
    msg_printf(MSG_MSG, "RAM buffer set to %u bytes.\n", e->ram_size);
    if (e->ram_size < 8192) {
        msg_printf(MSG_WARN, "Very small RAM buffer size. Is it a correct size ?\n");
    }
    if ((e->ram = malloc(e->ram_size)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for RAM buffer.\n");
        free(e->rom);
        free(tmp1);
        return 1;
    }
    for (int i = 0; i < e->ram_size; i++) {
        e->ram[i] = rand() & 0xff;
    }
    on_exit(file_free_buffers, e);
    free(tmp1);
    return 0;
}

/*******************************************/
static char file_set_cpu(const char* cfg, struct elwro* e, const char* profile) {
    char tmp0[512], *tmp1;
    if (file_find_cfg_option(profile, "cpu", tmp0, NULL, 0, 0) == NULL)
        return 1;
    msg_printf(MSG_MSG, "CPU section: %s\n", tmp0);
    if ((tmp1 = (char*)malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (file_find_cfg_section(cfg, tmp0, tmp1, NULL, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    if (file_find_cfg_option(tmp1, "call_fq", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->cpu_refresh = strtol(tmp0, NULL, 0);
    msg_printf(MSG_MSG, "CPU call: %i [ms]\n", e->cpu_refresh);
    if (file_find_cfg_option(tmp1, "atom", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->tr = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Set %i ticks/call\n", e->tr);
    if (file_find_cfg_option(tmp1, "intfq", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->ifreq = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "IRQ call: %i ticks of CPU\n", e->ifreq);
    free(tmp1);
    return 0;
}

/*******************************************/
static char file_set_screen(const char* cfg, const struct elwro* e, const char* profile) {
    char tmp0[1024], *tmp1;
    if (file_find_cfg_option(profile, "screen", tmp0, NULL, 0, 0) == NULL)
        return 1;
    msg_printf(MSG_MSG, "Screen section: %s\n", tmp0);
    if ((tmp1 = (char*)malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (file_find_cfg_section(cfg, tmp0, tmp1, NULL, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    if (file_find_cfg_option(tmp1, "blink", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->flash = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Blinking rate: %i [ms]\n", e->scn->flash);
    if (file_find_cfg_option(tmp1, "rfsh", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->rfsh = file_cfg_int(tmp0, 0);
    if (file_find_cfg_option(tmp1, "brdsize", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->bsize = (float)strtod(tmp0, NULL);
    msg_printf(MSG_MSG, "Border size: %f of window size \n", e->scn->bsize);
    msg_printf(MSG_MSG, "Refresh screen: %i [ms]\n", e->scn->rfsh);
    if (file_find_cfg_section(tmp1, "palette", tmp0, NULL, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    if (file_find_cfg_option(tmp0, "bright", tmp1, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->bright = file_cfg_int(tmp1, 0);
    msg_printf(MSG_MSG, "Palette brightness: %i \n", e->scn->bright);
    if (file_find_cfg_option(tmp0, "brt_normal", tmp1, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->nbrt = file_cfg_int(tmp1, 0);
    msg_printf(MSG_MSG, "Palette brightness 'normal': %i \n", e->scn->nbrt);
    if (file_find_cfg_option(tmp0, "brt_brt", tmp1, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->bbrt = file_cfg_int(tmp1, 0);
    msg_printf(MSG_MSG, "Palette bright 'bright': %i \n", e->scn->bbrt);
    if (file_find_cfg_option(tmp0, "brt_wht", tmp1, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->wbrt = file_cfg_int(tmp1, 0);
    msg_printf(MSG_MSG, "Palette bright white color: %i \n", e->scn->wbrt);

    free(tmp1);
    return 0;
}

/*******************************************/
static char file_set_video(const char* cfg, const struct elwro* e, const char* profile) {
    char tmp0[512], *tmp1;
    if (file_find_cfg_option(profile, "video", tmp0, NULL, 0, 0) == NULL)
        return 1;
    msg_printf(MSG_MSG, "Video section: %s\n", tmp0);
    if ((tmp1 = (char*)malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (file_find_cfg_section(cfg, tmp0, tmp1, NULL, 0) == NULL) {
        free(tmp1);
        return 1;
    }

    if (file_find_cfg_option(tmp1, "width", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->width = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Width: %i\n", e->scn->width);
    if (file_find_cfg_option(tmp1, "height", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->height = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Height: %i\n", e->scn->height);
    if (file_find_cfg_option(tmp1, "bpp", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->bpp = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "BitsPerPixel: %i\n", e->scn->bpp);
    if (file_find_cfg_option(tmp1, "fscrn", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->scn->fscrn = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Fullscreen: %i\n", e->scn->fscrn);

    free(tmp1);
    return 0;
}

/*******************************************/
static char file_set_sound(const char* cfg, struct elwro* e, const char* profile) {
    char tmp0[512], *tmp1;
    if (file_find_cfg_option(profile, "sound", tmp0, NULL, 0, 0) == NULL)
        return 1;
    msg_printf(MSG_MSG, "Sound section: %s\n", tmp0);
    if ((tmp1 = (char*)malloc(strlen(cfg) + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        return 1;
    }
    if (file_find_cfg_section(cfg, tmp0, tmp1, NULL, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    if (file_find_cfg_option(tmp1, "nosound", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->nosound = file_cfg_int(tmp0, 0) != 0;
    msg_printf(MSG_MSG, "Sound: %s\n", e->snd->nosound ? "off" : "on");
    if (e->snd->nosound) {
        free(tmp1);
        return 0;
    }
    if (file_find_cfg_option(tmp1, "channels", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->chn = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Channels: %i\n", e->snd->chn);
    if (file_find_cfg_option(tmp1, "rbuffer", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->size = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Sound round buffer size: %i\n", e->snd->size);
    if (file_find_cfg_option(tmp1, "samplert", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->freq = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Sampling freq: %i Hz\n", e->snd->freq);
    if (file_find_cfg_option(tmp1, "dbuffer", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->length = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Driver line buffer size: %i\n", e->snd->length);
    if (file_find_cfg_option(tmp1, "mixvol", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->volume = strtof(tmp0, NULL);
    msg_printf(MSG_MSG, "Mix volume: %g\n", e->snd->volume);
    if (file_find_cfg_option(tmp1, "scale", tmp0, NULL, 0, 0) == NULL) {
        free(tmp1);
        return 1;
    }
    e->snd->scale = file_cfg_int(tmp0, 0);
    msg_printf(MSG_MSG, "Max samples / CPU atom: %i\n", e->snd->scale);

    if (!file_find_cfg_option(tmp1, "alias", tmp0, NULL, 0, 0)) {
        free(tmp1);
        return 1;
    }
    e->snd->alias = file_cfg_int(tmp0, 0) != 0;
    msg_printf(MSG_MSG, "Wave antialias: %s\n", e->snd->alias ? "on" : "off");

    free(tmp1);
    return 0;
}

/*******************************************/
bool file_init(struct elwro* e, const char* config_path, const int argc, const char** argv) {
    char *cfg, *profile;
    int i;

    if (file_lookup_option(argc, argv, "-debug")) {
        e->debug = 1;
    }
    if ((i = file_lookup_option(argc, argv, "-cfg"))) {
        if (i + 1 >= argc) {
            msg_printf(MSG_ERROR, "Missing an argument for the -cfg option.\n");
            return false;
        }
        config_path = argv[i + 1];
    }
    struct stat st;
    if (stat(config_path, &st) != 0) {
        msg_printf(MSG_ERROR, "Config file does not exists: %s\n", config_path);
        return false;
    }
    if ((cfg = malloc(st.st_size + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error memory allocation for config file\n");
        return false;
    }
    FILE* f = fopen(config_path, "rb");
    if (f != NULL) {
        msg_printf(MSG_MSG, "Config file: %s\n", config_path);
        const size_t size = st.st_size;
        if (fread(cfg, 1, size, f) < size) {
            if (feof(f) != 0) {
                msg_printf(MSG_ERROR, "Unexpected EOF while reading the configuration file.\n");
            } else if (ferror(f) != 0) {
                msg_printf(MSG_ERROR, "Error occurred while reading the configuration file.\n");
            }
            fclose(f);
            return false;
        }
        fclose(f);
    } else {
        msg_printf(MSG_ERROR, "Error opening the config file: %s\n", config_path);
        return false;
    }
    if ((profile = malloc(st.st_size + 1)) == NULL) {
        msg_printf(MSG_ERROR, "Error allocate memory for configuration.\n");
        free(cfg);
        return false;
    }
    if (file_get_profile(cfg, profile) == NULL) {
        return false;
    }
    if (file_lookup_option(argc, argv, "-profile")) {
        //	strcpy(profile,argv[i+1]);
        //	msg_printf(MSG_MSG, "Forced profile by command line option to %s\n", profile);
        //        if(!file_find_cfg_section(cfg,tmp,profile,NULL,0)) { free(tmp); return NULL; }
    }
    if (file_memo_set(cfg, e, profile)) {
        return false;
    }
    if (file_bios_load(cfg, e->rom, profile, e->rom_size)) {
        return false;
    }
    if (file_set_cpu(cfg, e, profile)) {
        return false;
    }
    if (file_set_screen(cfg, e, profile)) {
        return false;
    }
    if (file_set_video(cfg, e, profile)) {
        return false;
    }
    if (file_set_sound(cfg, e, profile)) {
        return false;
    }

    /* dojdą dodatkowe*/
    free(profile);
    free(cfg);

    return true;
}

void file_option_param(const int argc, const char** argv, const int pos, const int min, const int max, int* val) {
    if (pos + 1 >= argc) {
        msg_printf(MSG_WARN, "Missing parameter to option %s\n", argv[pos]);
        return;
    }
    const int tmp = strtol(argv[pos + 1], NULL, 0);
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
