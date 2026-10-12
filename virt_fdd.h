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

#ifndef _virt_fdd_h_
#define _virt_fdd_h_

#ifndef uchar
#define uchar unsigned char
#endif

typedef struct {
    char format_name[0x22]; /* 00-21 */
    char creator[14]; /* 22-2F */
    uchar tps; /* ilość ścieżek */
    uchar hpd; /* ilość głowic  */
    uchar size_1;
    uchar size_2; /* rozmiar ścieżki */
    uchar unused_2[0xff - 0x34 + 1];
} vfdd_hdr;

typedef struct {
    char info[13]; /* Track-Info\r\n */
    char unused_0[3];
    uchar tr_nb; /* numer ścieżki */
    uchar hd_nb; /* numer głowicy */
    char unused_1[2];
    /* parametry formatowania ścieżki */
    uchar bps; /* bytes per sector */
    uchar spt; /* sectors per track */
    uchar gap3; /* gap#3 format */
    uchar fill; /* bajt wypełnienia */
} vfdd_trk_hdr;

typedef struct {
    uchar tr_nb;
    uchar hd_nb;
    uchar sc_nb;
    uchar bps;
    uchar err_1;
    uchar err_2; /* 0x40 - deleted, */
    short int sec_len; /* little endian */
} vfdd_sec_hdr;

typedef struct {
    FILE* f; /* uchwyt czytanego pliku obrazu */
    vfdd_hdr hdr; /* nagłówek obrazu dysku */
    vfdd_trk_hdr th;
    vfdd_sec_hdr sh;
    int sbyte;
    int lbytes;
    long seek;
    int stat;
    int emulation; /* rodzaj emulacji */
} vfdd_disk;

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

/* póki nie ma innych emulacji napędu niż pliki *.dsk */
#define vfdd_floppy vfdd_dsk_floppy

/*
    Emulacja Dyskietki
    1 - dla akcji VFDD_WR_SEC
    2 - dla akcji VFDD_LOAD
*/
extern int vfdd_dsk_floppy(
    vfdd_disk*,
    char action,
    char parm0, /* ścieżka | 1-dana do zapisu | 2-tryb otwarcia (RO/RW) */
    char parm1, /* sektor  | 2-typ emulacji */
    char parm2, /* głowica */
    char* path /* ścieżka do pliku/katalogu */
);

/*
    Emulacja napędu
*/
extern int vfdd_drive(
    void* ptr, /* wskaźnik nadrzędnej struktury danych */
    void* dsk, /* vfdd_disk */
    uchar action, /* rodzaj akcji - zdefiniowane w  i8272_fdc.h */
    uchar wdata /* dana dla operacji zapisu */
);

#endif
