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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "e800j_config.h"
#include "e800j_cfg.h"
#include "e800j.h"
#include "e800j_msg.h"
#include "e800j_sound.h"
#include "zx_scrn.h"

/****************************************************************/
/* parser pliku konfiguracji                                    */
/****************************************************************/

struct parser {
    const char* p;
    const char* path;
    int line;
};

static void parse_error(const struct parser* ps, const int line, const char* what, const char* arg) {
    msg_printf(MSG_CRITICAL_ERROR, "Config file %s, line %i: %s%s\n", ps->path, line, what, arg);
}

static bool is_name_char(const int c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        c == '_' || c == '-' || c == '.';
}

/* pomija białe znaki i komentarze (# do końca linii) */
static void parse_skip(struct parser* ps) {
    for (;;) {
        const char c = *ps->p;
        if (c == '\n') {
            ps->line++;
            ps->p++;
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v') {
            ps->p++;
        } else if (c == '#') {
            while (*ps->p != 0 && *ps->p != '\n')
                ps->p++;
        } else {
            return;
        }
    }
}

static char* dup_n(const char* s, const size_t n) {
    char* r = malloc(n + 1);
    if (r != nullptr) {
        memcpy(r, s, n);
        r[n] = 0;
    }
    return r;
}

static struct cfg_section* add_section(struct cfg_section* parent, char* name, const int line) {
    struct cfg_section* arr = realloc(parent->sections, (parent->section_count + 1) * sizeof(*arr));
    if (arr == nullptr) {
        free(name);
        return nullptr;
    }
    parent->sections = arr;
    struct cfg_section* s = &arr[parent->section_count++];
    memset(s, 0, sizeof(*s));
    s->name = name;
    s->line = line;
    return s;
}

static bool add_entry(struct cfg_section* sec, char* key, char* value, const int line) {
    struct cfg_entry* arr = realloc(sec->entries, (sec->entry_count + 1) * sizeof(*arr));
    if (arr == nullptr) {
        free(key);
        free(value);
        return false;
    }
    sec->entries = arr;
    arr[sec->entry_count].key = key;
    arr[sec->entry_count].value = value;
    arr[sec->entry_count].line = line;
    sec->entry_count++;
    return true;
}

/* wartość: "w cudzysłowie" (\" i \\ to sekwencje specjalne, inne ukośniki zostają) albo goła do ';' */
static char* parse_value(struct parser* ps, const int line) {
    if (*ps->p == '"') {
        const char* s = ++ps->p;
        size_t len = 0;
        while (*ps->p != '"') {
            if (*ps->p == 0 || *ps->p == '\n') {
                parse_error(ps, line, "unterminated string", "");
                return nullptr;
            }
            if (*ps->p == '\\' && (ps->p[1] == '"' || ps->p[1] == '\\'))
                ps->p++;
            ps->p++;
            len++;
        }
        char* r = malloc(len + 1);
        if (r == nullptr)
            return nullptr;
        size_t k = 0;
        for (const char* q = s; q < ps->p; q++) {
            if (*q == '\\' && (q[1] == '"' || q[1] == '\\'))
                q++;
            r[k++] = *q;
        }
        r[k] = 0;
        ps->p++; /* zamykający cudzysłów */
        return r;
    }
    const char* s = ps->p;
    while (*ps->p != 0 && *ps->p != ';' && *ps->p != '\n' && *ps->p != '#' && *ps->p != '{' && *ps->p != '}')
        ps->p++;
    const char* e = ps->p;
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r'))
        e--;
    if (e == s) {
        parse_error(ps, line, "missing value", "");
        return nullptr;
    }
    return dup_n(s, (size_t)(e - s));
}

static bool parse_body(struct parser* ps, struct cfg_section* sec, const int depth) {
    for (;;) {
        parse_skip(ps);
        const char c = *ps->p;
        if (c == 0) {
            if (depth > 0) {
                parse_error(ps, ps->line, "missing '}' to close section ", sec->name);
                return false;
            }
            return true;
        }
        if (c == '}') {
            if (depth == 0) {
                parse_error(ps, ps->line, "unexpected '}'", "");
                return false;
            }
            ps->p++;
            return true;
        }

        const int line = ps->line;
        const char* start = ps->p;
        while (is_name_char(*ps->p))
            ps->p++;
        const size_t len = (size_t)(ps->p - start);
        if (len == 0) {
            char bad[2] = { c, 0 };
            parse_error(ps, line, "unexpected character ", bad);
            return false;
        }
        parse_skip(ps);

        if (len == 7 && strncmp(start, "section", 7) == 0 && is_name_char(*ps->p)) {
            const char* ns = ps->p;
            while (is_name_char(*ps->p))
                ps->p++;
            char* name = dup_n(ns, (size_t)(ps->p - ns));
            if (name == nullptr)
                return false;
            parse_skip(ps);
            if (*ps->p != '{') {
                parse_error(ps, line, "missing '{' to open section ", name);
                free(name);
                return false;
            }
            ps->p++;
            struct cfg_section* sub = add_section(sec, name, line);
            if (sub == nullptr || !parse_body(ps, sub, depth + 1))
                return false;
            continue;
        }

        char* key = dup_n(start, len);
        if (key == nullptr)
            return false;
        if (*ps->p != '=') {
            parse_error(ps, line, "expected '=' after ", key);
            free(key);
            return false;
        }
        ps->p++;
        parse_skip(ps);
        char* value = parse_value(ps, line);
        if (value == nullptr) {
            free(key);
            return false;
        }
        parse_skip(ps);
        if (*ps->p != ';') {
            parse_error(ps, line, "missing ';' after the value of ", key);
            free(key);
            free(value);
            return false;
        }
        ps->p++;
        if (!add_entry(sec, key, value, line))
            return false;
    }
}

bool cfg_parse(const char* text, const char* path, struct cfg_section* root) {
    memset(root, 0, sizeof(*root));
    struct parser ps = { text, path, 1 };
    if (strncmp(text, "\xEF\xBB\xBF", 3) == 0) /* BOM UTF-8 */
        ps.p += 3;
    return parse_body(&ps, root, 0);
}

const struct cfg_section* cfg_find_section(const struct cfg_section* parent, const char* name) {
    for (size_t i = 0; i < parent->section_count; i++)
        if (strcmp(parent->sections[i].name, name) == 0)
            return &parent->sections[i];
    return nullptr;
}

const char* cfg_find_value(const struct cfg_section* section, const char* key) {
    for (size_t i = 0; i < section->entry_count; i++)
        if (strcmp(section->entries[i].key, key) == 0)
            return section->entries[i].value;
    return nullptr;
}

static void cfg_free_contents(struct cfg_section* s) {
    for (size_t i = 0; i < s->entry_count; i++) {
        free(s->entries[i].key);
        free(s->entries[i].value);
    }
    free(s->entries);
    for (size_t i = 0; i < s->section_count; i++)
        cfg_free_contents(&s->sections[i]);
    free(s->sections);
    free(s->name);
    memset(s, 0, sizeof(*s));
}

void cfg_free(struct cfg_section* root) {
    cfg_free_contents(root);
}

/****************************************************************/
/* kontekst konfiguracji                                        */
/****************************************************************/

void config_init(struct config* cfg) {
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->path, sizeof(cfg->path), "%s", "e800j.cfg");
    cfg->rom_bank_size = 0x6000;
    cfg->ram_bank_size = 0x10000;
    cfg->call_fq = 8;
    cfg->atom = 28320;
    cfg->intfq = 70800;
    cfg->blink = 320;
    cfg->rfsh = 40;
    cfg->brdsize = 0.1f;
    cfg->osd_time_ms = 10000;
    cfg->bright = 0x00;
    cfg->brt_normal = 0xa0;
    cfg->brt_bright = 0xf0;
    cfg->brt_white = 0xe0;
    cfg->width = 640;
    cfg->height = 480;
    cfg->bpp = 0;
    cfg->channels = 1;
    cfg->samplert = 44100;
    cfg->dbuffer = 512;
    cfg->mixvol = 128.f;
    cfg->alias = true;
    cfg->msg_level = -1;
}

/* wczytuje cały plik do bufora zakończonego zerem; *size = długość pliku */
static char* read_file(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (f == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Cannot open file: %s\n", path);
        return nullptr;
    }
    long length = -1;
    if (fseek(f, 0, SEEK_END) == 0)
        length = ftell(f);
    if (length < 0 || fseek(f, 0, SEEK_SET) != 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Cannot determine the size of file: %s\n", path);
        fclose(f);
        return nullptr;
    }
    char* buf = malloc((size_t)length + 1);
    if (buf == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Not enough memory to read file: %s\n", path);
        fclose(f);
        return nullptr;
    }
    if (fread(buf, 1, (size_t)length, f) != (size_t)length) {
        msg_printf(MSG_CRITICAL_ERROR, "Error reading file: %s\n", path);
        free(buf);
        fclose(f);
        return nullptr;
    }
    fclose(f);
    buf[length] = 0;
    *size = (size_t)length;
    return buf;
}

/* --- odczyt wartości z sekcji; brak klucza zostawia wartość domyślną, błędna wartość to błąd --- */

static const struct cfg_entry* find_entry(const struct cfg_section* s, const char* key) {
    for (size_t i = 0; i < s->entry_count; i++)
        if (strcmp(s->entries[i].key, key) == 0)
            return &s->entries[i];
    return nullptr;
}

static bool get_number(const struct cfg_section* s, const char* key, const double min, const double max, double* out, const bool integer) {
    const struct cfg_entry* en = find_entry(s, key);
    if (en == nullptr)
        return true;
    char* end;
    errno = 0;
    double v;
    if (integer)
        v = (double)strtol(en->value, &end, 0);
    else
        v = strtod(en->value, &end);
    if (end == en->value || *end != 0 || errno != 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, option %s (line %i): not a number: %s\n", s->name, key, en->line, en->value);
        return false;
    }
    if (v < min || v > max) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, option %s (line %i): value out of range: %s\n", s->name, key, en->line, en->value);
        return false;
    }
    *out = v;
    return true;
}

static bool get_int(const struct cfg_section* s, const char* key, const int min, const int max, int* out) {
    double v = *out;
    if (!get_number(s, key, min, max, &v, true))
        return false;
    *out = (int)v;
    return true;
}

static bool get_float(const struct cfg_section* s, const char* key, const double min, const double max, float* out) {
    double v = *out;
    if (!get_number(s, key, min, max, &v, false))
        return false;
    *out = (float)v;
    return true;
}

static bool get_bool(const struct cfg_section* s, const char* key, bool* out) {
    const struct cfg_entry* en = find_entry(s, key);
    if (en == nullptr)
        return true;
    static const char* const yes[] = { "1", "true", "yes", "on" };
    static const char* const no[] = { "0", "false", "no", "off" };
    for (int i = 0; i < 4; i++) {
        if (strcmp(en->value, yes[i]) == 0) {
            *out = true;
            return true;
        }
        if (strcmp(en->value, no[i]) == 0) {
            *out = false;
            return true;
        }
    }
    msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, option %s (line %i): not a boolean (0/1, yes/no, on/off, true/false): %s\n", s->name, key, en->line, en->value);
    return false;
}

static bool get_string(const struct cfg_section* s, const char* key, char* dst, const size_t size) {
    const struct cfg_entry* en = find_entry(s, key);
    if (en == nullptr)
        return true;
    if (strlen(en->value) >= size) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, option %s (line %i): value too long\n", s->name, key, en->line);
        return false;
    }
    memcpy(dst, en->value, strlen(en->value) + 1);
    return true;
}

/* sekcja wskazana opcją profilu (np. cpu = Z80): 1 - znaleziona, 0 - profil nie ma tej opcji, -1 - błąd */
static int get_ref(const struct cfg_section* root, const struct cfg_section* prof, const char* key, const struct cfg_section** out) {
    const struct cfg_entry* en = find_entry(prof, key);
    if (en == nullptr)
        return 0;
    *out = cfg_find_section(root, en->value);
    if (*out == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: profile %s, option %s (line %i): section not found: %s\n", prof->name, key, en->line, en->value);
        return -1;
    }
    return 1;
}

static bool load_bios(const struct cfg_section* root, const struct cfg_section* prof, struct config* cfg) {
    const struct cfg_section* bios;
    const int r = get_ref(root, prof, "rombios", &bios);
    if (r < 0)
        return false;
    if (r == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: profile %s has no 'rombios' option\n", prof->name);
        return false;
    }
    const char* set_name = cfg_find_value(bios, "set_bios");
    if (set_name == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: section %s has no 'set_bios' option\n", bios->name);
        return false;
    }
    if (strlen(set_name) >= sizeof(cfg->bios_set)) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: set_bios value too long: %s\n", set_name);
        return false;
    }
    memcpy(cfg->bios_set, set_name, strlen(set_name) + 1);
    msg_printf(MSG_MSG, "ROM chosen: %s\n", cfg->bios_set);
    const struct cfg_section* set = cfg_find_section(bios, cfg->bios_set);
    if (set == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: ROM set not found in section %s: %s\n", bios->name, cfg->bios_set);
        return false;
    }

    /* pary address / file */
    int address = 0;
    bool have_address = false;
    cfg->rom_count = 0;
    for (size_t i = 0; i < set->entry_count; i++) {
        const struct cfg_entry* en = &set->entries[i];
        if (strcmp(en->key, "address") == 0) {
            char* end;
            errno = 0;
            const long v = strtol(en->value, &end, 0);
            if (end == en->value || *end != 0 || errno != 0 || v < 0 || v > 0xFFFFFF) {
                msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, line %i: invalid address: %s\n", set->name, en->line, en->value);
                return false;
            }
            address = (int)v;
            have_address = true;
        } else if (strcmp(en->key, "file") == 0) {
            if (!have_address) {
                msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, line %i: 'file' without a preceding 'address'\n", set->name, en->line);
                return false;
            }
            if (cfg->rom_count >= CFG_ROM_MAX) {
                msg_printf(MSG_CRITICAL_ERROR, "Config: section %s: too many ROM files (max %i)\n", set->name, CFG_ROM_MAX);
                return false;
            }
            if (strlen(en->value) >= sizeof(cfg->rom[0].file)) {
                msg_printf(MSG_CRITICAL_ERROR, "Config: section %s, line %i: file name too long\n", set->name, en->line);
                return false;
            }
            cfg->rom[cfg->rom_count].address = address;
            memcpy(cfg->rom[cfg->rom_count].file, en->value, strlen(en->value) + 1);
            cfg->rom_count++;
            have_address = false;
        }
    }
    if (cfg->rom_count == 0) {
        msg_printf(MSG_CRITICAL_ERROR, "Config: ROM set %s has no files\n", set->name);
        return false;
    }
    return true;
}

static bool load_memory(const struct cfg_section* root, const struct cfg_section* prof, struct config* cfg) {
    const struct cfg_section* s;
    const int r = get_ref(root, prof, "memory", &s);
    if (r <= 0)
        return r == 0;
    return get_int(s, "rom_bank_size", 1, 0x1000000, &cfg->rom_bank_size) &&
        get_int(s, "ram_bank_size", 0x6000, 0x1000000, &cfg->ram_bank_size);
}

static bool load_cpu(const struct cfg_section* root, const struct cfg_section* prof, struct config* cfg) {
    const struct cfg_section* s;
    const int r = get_ref(root, prof, "cpu", &s);
    if (r <= 0)
        return r == 0;
    return get_int(s, "call_fq", 1, 1000, &cfg->call_fq) &&
        get_int(s, "atom", 1, 100000000, &cfg->atom) &&
        get_int(s, "intfq", 1, 100000000, &cfg->intfq);
}

static bool load_screen(const struct cfg_section* root, const struct cfg_section* prof, struct config* cfg) {
    const struct cfg_section* s;
    const int r = get_ref(root, prof, "screen", &s);
    if (r <= 0)
        return r == 0;
    if (!get_int(s, "blink", 1, 100000, &cfg->blink) ||
        !get_int(s, "rfsh", 1, 100000, &cfg->rfsh) ||
        !get_float(s, "brdsize", 0.0, 1.0, &cfg->brdsize))
        return false;
    /* osd_time: sekundy, 0 -> wyłączone */
    float seconds = (float)cfg->osd_time_ms / 1000.f;
    if (!get_float(s, "osd_time", 0.0, 3600.0, &seconds))
        return false;
    cfg->osd_time_ms = (int)(seconds * 1000.f);
    const struct cfg_section* pal = cfg_find_section(s, "palette");
    if (pal != nullptr) {
        return get_int(pal, "bright", 0, 255, &cfg->bright) &&
            get_int(pal, "brt_normal", 0, 255, &cfg->brt_normal) &&
            get_int(pal, "brt_brt", 0, 255, &cfg->brt_bright) &&
            get_int(pal, "brt_wht", 0, 255, &cfg->brt_white);
    }
    return true;
}

static bool load_video(const struct cfg_section* root, const struct cfg_section* prof, struct config* cfg) {
    const struct cfg_section* s;
    const int r = get_ref(root, prof, "video", &s);
    if (r <= 0)
        return r == 0;
    return get_int(s, "width", 1, 16384, &cfg->width) &&
        get_int(s, "height", 1, 16384, &cfg->height) &&
        get_int(s, "bpp", 0, 32, &cfg->bpp) &&
        get_bool(s, "fscrn", &cfg->fullscreen);
}

static bool load_sound(const struct cfg_section* root, const struct cfg_section* prof, struct config* cfg) {
    const struct cfg_section* s;
    const int r = get_ref(root, prof, "sound", &s);
    if (r <= 0)
        return r == 0;
    return get_bool(s, "nosound", &cfg->nosound) &&
        get_int(s, "channels", 1, 2, &cfg->channels) &&
        get_int(s, "samplert", 8000, 192000, &cfg->samplert) &&
        get_int(s, "dbuffer", 16, 65536, &cfg->dbuffer) &&
        get_float(s, "mixvol", 0.0, SND_VOL_MAX, &cfg->mixvol) &&
        get_bool(s, "alias", &cfg->alias);
}

/* katalog dyskietek z profilu; końcowe ukośniki są obcinane */
static bool load_diskdir(const struct cfg_section* prof, struct config* cfg) {
    if (!get_string(prof, "diskdir", cfg->diskdir, sizeof(cfg->diskdir)))
        return false;
    size_t length = strlen(cfg->diskdir);
    while (length > 1 && (cfg->diskdir[length - 1] == '/' || cfg->diskdir[length - 1] == '\\'))
        cfg->diskdir[--length] = 0;
    if (length > 0)
        msg_printf(MSG_MSG, "Disk directory: %s\n", cfg->diskdir);
    return true;
}

bool config_load(struct config* cfg) {
    size_t size;
    char* text = read_file(cfg->path, &size);
    if (text == nullptr)
        return false;
    msg_printf(MSG_MSG, "Config file: %s\n", cfg->path);

    struct cfg_section root;
    bool ok = cfg_parse(text, cfg->path, &root);
    free(text);
    if (!ok) {
        cfg_free(&root);
        return false;
    }

    const struct cfg_section* prof = nullptr;
    do {
        ok = false;
        if (cfg->profile[0] != 0) {
            msg_printf(MSG_MSG, "Chosen profile (command line): %s\n", cfg->profile);
        } else {
            const struct cfg_section* main_section = cfg_find_section(&root, "main");
            const char* name = main_section != nullptr ? cfg_find_value(main_section, "profile") : nullptr;
            if (name == nullptr) {
                msg_printf(MSG_CRITICAL_ERROR, "Config file %s: no 'profile' option in section main (use --profile)\n", cfg->path);
                break;
            }
            if (strlen(name) >= sizeof(cfg->profile)) {
                msg_printf(MSG_CRITICAL_ERROR, "Config file %s: profile name too long: %s\n", cfg->path, name);
                break;
            }
            memcpy(cfg->profile, name, strlen(name) + 1);
            msg_printf(MSG_MSG, "Chosen profile: %s\n", cfg->profile);
        }
        prof = cfg_find_section(&root, cfg->profile);
        if (prof == nullptr) {
            msg_printf(MSG_CRITICAL_ERROR, "Profile section not found in the config file %s: %s\n", cfg->path, cfg->profile);
            break;
        }
        ok = load_bios(&root, prof, cfg) &&
            load_memory(&root, prof, cfg) &&
            load_cpu(&root, prof, cfg) &&
            load_screen(&root, prof, cfg) &&
            load_video(&root, prof, cfg) &&
            load_sound(&root, prof, cfg) &&
            load_diskdir(prof, cfg);
    } while (0);

    cfg_free(&root);
    return ok;
}

static bool load_rom_file(const char* path, const int address, unsigned char* rom, const int rom_size) {
    size_t size;
    char* data = read_file(path, &size);
    if (data == nullptr)
        return false;
    if ((size_t)address + size > (size_t)rom_size) {
        msg_printf(MSG_CRITICAL_ERROR, "File %s is too large for the ROM buffer (0x%x bytes) at address 0x%x\n", path, (unsigned)rom_size, (unsigned)address);
        free(data);
        return false;
    }
    memcpy(rom + address, data, size);
    free(data);
    return true;
}

bool config_apply(const struct config* cfg, struct elwro* e) {
    e->rom_size = cfg->rom_bank_size;
    e->ram_size = cfg->ram_bank_size;
    msg_printf(MSG_MSG, "ROM buffer set to %u bytes.\n", (unsigned)e->rom_size);
    msg_printf(MSG_MSG, "RAM buffer set to %u bytes.\n", (unsigned)e->ram_size);
    if ((e->rom = malloc((size_t)e->rom_size)) == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Error allocate memory for ROM buffer.\n");
        return false;
    }
    if ((e->ram = malloc((size_t)e->ram_size)) == nullptr) {
        msg_printf(MSG_CRITICAL_ERROR, "Error allocate memory for RAM buffer.\n");
        free(e->rom);
        e->rom = nullptr;
        return false;
    }
    for (int i = 0; i < e->rom_size; i++)
        e->rom[i] = (unsigned char)rand();
    for (int i = 0; i < e->ram_size; i++)
        e->ram[i] = (unsigned char)(rand() & 0xff);

    for (int i = 0; i < cfg->rom_count; i++) {
        msg_printf(MSG_MSG, "Loading file %s to ROM banks at the address 0x%X...\n", cfg->rom[i].file, (unsigned)cfg->rom[i].address);
        if (!load_rom_file(cfg->rom[i].file, cfg->rom[i].address, e->rom, e->rom_size))
            return false;
    }

    e->cpu_refresh = (unsigned int)cfg->call_fq;
    e->tr = cfg->atom;
    e->ifreq = cfg->intfq;
    msg_printf(MSG_MSG, "CPU call: %i [ms], %i ticks/call, IRQ every %i ticks\n", cfg->call_fq, cfg->atom, cfg->intfq);

    struct screen* scn = e->scn;
    scn->flash = cfg->blink;
    scn->rfsh = cfg->rfsh;
    scn->bsize = cfg->brdsize;
    scn->toast_ms = cfg->osd_time_ms;
    scn->bright = cfg->bright;
    scn->nbrt = cfg->brt_normal;
    scn->bbrt = cfg->brt_bright;
    scn->wbrt = cfg->brt_white;
    scn->width = cfg->width;
    scn->height = cfg->height;
    scn->bpp = cfg->bpp;
    scn->fscrn = cfg->fullscreen;
    scn->noframe = cfg->noframe;
    msg_printf(MSG_MSG, "Window: %ix%i, %i bpp, fullscreen: %i\n", cfg->width, cfg->height, cfg->bpp, cfg->fullscreen);

    struct sound* snd = e->snd;
    snd->nosound = cfg->nosound;
    snd->chn = cfg->channels;
    snd->freq = cfg->samplert;
    snd->length = cfg->dbuffer;
    snd->volume = cfg->mixvol;
    snd->alias = cfg->alias;
    msg_printf(MSG_MSG, "Sound: %s\n", cfg->nosound ? "off" : "on");

    snprintf(e->diskdir, sizeof(e->diskdir), "%s", cfg->diskdir);
    snprintf(e->snap_path, sizeof(e->snap_path), "%s", cfg->snapshot);
    e->debug = cfg->debug;
    return true;
}
