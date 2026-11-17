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

struct vfdd_hdr {
    char format_name[0x22];     // 00-21
    char creator[14];           // 22-2F
    unsigned char tps;          // ilość ścieżek
    unsigned char hpd;          // ilość głowic
    unsigned char size_1;
    unsigned char size_2;       // rozmiar ścieżki
    unsigned char unused_2[0xff - 0x34 + 1];
};

struct vfdd_trk_hdr {
    char info[13];              // Track-Info\r\n
    char unused_0[3];
    unsigned char tr_nb;        // numer ścieżki
    unsigned char hd_nb;        // numer głowicy
    char unused_1[2];
    /* parametry formatowania ścieżki */
    unsigned char bps;          // bytes per sector
    unsigned char spt;          // sectors per track
    unsigned char gap3;         // gap#3 format
    unsigned char fill;         // bajt wypełnienia
};

struct vfdd_sec_hdr {
    unsigned char tr_nb;
    unsigned char hd_nb;
    unsigned char sc_nb;
    unsigned char bps;
    unsigned char err_1;
    unsigned char err_2;        // 0x40 - deleted
    short sec_len;              // little endian
};

struct vfdd_disk {
    FILE* f;                    // uchwyt czytanego pliku obrazu
    struct vfdd_hdr hdr;        // nagłówek obrazu dysku
    struct vfdd_trk_hdr th;
    struct vfdd_sec_hdr sh;
    int sbyte;
    int lbytes;
    long seek;
    int stat;
    int emulation;              // rodzaj emulacji
    char path[1024];            // ścieżka ostatnio wczytanego obrazu (zachowana po wysunięciu)
    unsigned char rw;           // żądany tryb otwarcia (VFDD_RO/VFDD_RW), zachowany po wysunięciu
};

enum {
    VFDD_DOPEN = 1,
    VFDD_WPROT = 2,
};

enum {
    VFDD_RO = 0,
    VFDD_RW = 1,
};

/* akcje, dla vfdd_dsk_floppy */
enum {
    VFDD_INIT,
    VFDD_LOAD,
    VFDD_UNLOAD,
    VFDD_SEEK_TRK,
    VFDD_SEEK_SEC,
    VFDD_RD_SEC,
    VFDD_WR_SEC,
};

enum {
    VFDD_EMUL_DSK, /* emulacja zawartości dyskietki w pliku CPCEmu formatu DSK */
    VFDD_EMUL_DIR, /* emulacja zawartości dyskietki katalogiem na HDD */
    VFDD_EMUL_REAL /* emulacja rzeczywistego napędu przez stację FDD  */
};

/*
    Emulacja Dyskietki
    1 - dla akcji VFDD_WR_SEC
    2 - dla akcji VFDD_LOAD
*/
int vfdd_dsk_floppy(
    struct vfdd_disk*,
    char action,
    unsigned char parm0, /* ścieżka | 1-dana do zapisu | 2-tryb otwarcia (RO/RW) */
    unsigned char parm1, /* sektor  | 2-typ emulacji */
    unsigned char parm2, /* głowica */
    const char* path /* ścieżka do pliku/katalogu */
);

/* ograniczenia formatu DSK: N sektora (0x80 << N) i liczba sektorów mieszcząca się w nagłówku ścieżki */
enum {
    VFDD_MAX_N = 6,
    VFDD_MAX_SPT = 29,
};

/* parametry rozkazu FORMAT A TRACK */
struct vfdd_fmt {
    unsigned char trk;              // numer fizycznej ścieżki
    unsigned char hd;               // głowica
    unsigned char n;                // kod rozmiaru sektora (dane), jednakowy dla całej ścieżki
    unsigned char sc;               // ilość sektorów
    unsigned char gap3;
    unsigned char filler;           // bajt wypełnienia danych
    const unsigned char* ids;       // sc * {C, H, R, N}: pola ID sektorów w kolejności na ścieżce
};

/*
    Formatowanie ścieżki: tworzy ją lub zastępuje (w kolejności ścieżka/głowica); jeśli rozmiar
    się zmienia, dalsze ścieżki są przesuwane, a plik skracany lub wydłużany.
    Zwraca FDD_OK albo FDD_ERR_NODISK / FDD_ERR_WPROT / FDD_ERR_OVER (błędne parametry lub błąd zapisu).
*/
int vfdd_format_track(struct vfdd_disk*, const struct vfdd_fmt*);

/*
    Tworzy (lub zeruje) plik z pustym, niesformatowanym obrazem: sam nagłówek, bez ścieżek.
    Ścieżki powstają dopiero rozkazem FORMAT A TRACK. Zwraca FDD_OK albo -1.
*/
int vfdd_create_image(const char* path);

/*
    Wymiana dyskietki: najpierw otwiera nowy obraz i dopiero po sukcesie zamyka stary,
    więc przy błędzie (-1) poprzednia dyskietka pozostaje we flopie. Dozwolone path == dsk->path.
*/
int vfdd_swap(struct vfdd_disk*, const char* path, unsigned char write_mode);

/*
    Emulacja napędu
*/
int vfdd_drive(
    const void* ptr, /* wskaźnik nadrzędnej struktury danych */
    void* dsk, /* vfdd_disk */
    unsigned char action, /* rodzaj akcji, zdefiniowane w i8272_fdc.h */
    unsigned char wdata /* dana dla operacji zapisu */
);