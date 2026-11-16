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
#include <stddef.h>

struct elwro;

/*
 Parser pliku konfiguracji (składnia: section nazwa { klucz = wartość; section podsekcja { ... } },
 komentarze od '#' do końca linii, wartości w cudzysłowie lub "gołe" do średnika) buduje drzewo
 sekcji. Wartości w cudzysłowie są zapisane bez cudzysłowów. Klucze w sekcji mogą się powtarzać
 (np. pary address/file w sekcji BIOS) - kolejność jest zachowana.
*/
struct cfg_entry {
    char* key;
    char* value;
    int line;
};

struct cfg_section {
    char* name;
    struct cfg_entry* entries;
    size_t entry_count;
    struct cfg_section* sections;
    size_t section_count;
    int line;
};

/* Parsuje tekst (zakończony zerem); path służy tylko w komunikatach. Błędy (z numerem linii) idą przez msg_printf. */
bool cfg_parse(const char* text, const char* path, struct cfg_section* root);

/* Pierwsza podsekcja o danej nazwie lub nullptr */
const struct cfg_section* cfg_find_section(const struct cfg_section* parent, const char* name);

/* Wartość pierwszego klucza o danej nazwie lub nullptr */
const char* cfg_find_value(const struct cfg_section* section, const char* key);

/* Zwalnia drzewo (nie sam root) */
void cfg_free(struct cfg_section* root);

/*
 Kontekst konfiguracji emulatora: wartości z pliku konfiguracji (wybrany profil) uzupełnione
 o opcje z linii poleceń. Jest wypełniany w kolejności: domyślne -> plik konfiguracji ->
 linia poleceń (cmdline_parse nadpisuje wartości z pliku), a potem stosowany przez config_apply().
*/
#define CFG_PATH_MAX 512
#define CFG_ROM_MAX 8

struct config {
    /* skąd */
    char path[CFG_PATH_MAX];    /* plik konfiguracji (--config) */
    char profile[64];           /* nazwa profilu (--profile); pusta = main::profile; po wczytaniu - użyty profil */

    /* BIOS / ROM */
    char bios_set[64];          /* set_bios: nazwa wybranego zestawu */
    int rom_count;
    struct {
        int address;
        char file[CFG_PATH_MAX];
    } rom[CFG_ROM_MAX];

    /* pamięć */
    int rom_bank_size;
    int ram_bank_size;

    /* CPU */
    int call_fq;                /* co ile ms wywoływana jest pętla procesora */
    int atom;                   /* taktów CPU na jedno wywołanie pętli */
    int intfq;                  /* co ile taktów przerwanie maskowalne */

    /* ekran */
    int blink;                  /* ms */
    int rfsh;                   /* ms */
    float brdsize;              /* szerokość ramki względem okna */
    int osd_time_ms;            /* czas komunikatów o operacjach, 0 = wyłączone */
    int bright, brt_normal, brt_bright, brt_white;

    /* obraz SDL */
    int width, height, bpp;     /* -x, -y, --bits-per-pixel nadpisują */
    bool fullscreen;            /* -f */
    bool noframe;               /* -n */

    /* dźwięk */
    bool nosound;
    int channels, samplert, dbuffer;
    float mixvol;
    bool alias;

    /* dyskietki */
    char diskdir[CFG_PATH_MAX]; /* diskdir z profilu */
    char disk[2][CFG_PATH_MAX]; /* -a/-A, -b/-B: obraz w napędzie A/B, pusty = brak */
    bool disk_rw[2];            /* true dla -A/-B (do zapisu), false dla -a/-b (tylko do odczytu) */

    /* tylko linia poleceń */
    char snapshot[CFG_PATH_MAX]; /* argument pozycyjny */
    bool help;                   /* -h, --help */
    bool debug;                  /* --debug */
    int msg_level;               /* --log-level, -1 = nie podano */
};

/* Ustawia wartości domyślne (zgodne z dostarczanym e800j.cfg); path = "e800j.cfg" */
void config_init(struct config* cfg);

/* Wczytuje plik cfg->path i wybrany profil do cfg (profil z cfg->profile lub main::profile).
   Zwraca false (komunikat przez msg_printf) przy błędzie pliku, składni lub wartości. */
bool config_load(struct config* cfg);

/* Przenosi kontekst do emulatora: alokuje i wypełnia ROM/RAM, wczytuje pliki ROM, ustawia
   CPU, ekran (e->scn), dźwięk (e->snd), katalog dyskietek, snapshot i tryb debuggera.
   e->scn i e->snd muszą już wskazywać na struktury. */
bool config_apply(const struct config* cfg, struct elwro* e);
