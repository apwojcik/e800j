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

struct i8272opc {
    void (*command)(void*); // funkcja obsługi
    unsigned char cmd_len; // ilość bajtów rozkazu
    unsigned char exe_len; // faza execute ?
    unsigned char res_len; // ilość bajtów fazy result
    char* name;
};

struct i8272 {
    /* rejestry układu i8272A */
    unsigned char msr; // main status register
    unsigned char st0; // rejestr statusowy 0
    unsigned char st1; // rejestr statusowy 1
    unsigned char st2; // rejestr statusowy 2
    unsigned char st3; // rejestr statusowy 3

    /* parametry wewnętrzne FDC */
    unsigned char srt; // "Step Rate Time"
    unsigned char hut; // "Head Unload Time "
    unsigned char hlt; // "Head Load Time"
    unsigned char hds; // "HeaD Select"
    unsigned char pcn; // "Present Cylinder Number"
    unsigned char ds; // "Drive Select"

    /* zmienne interpretacji rozkazu */
    unsigned char call; // miejsce wywołania opcode
    unsigned char icntr; // licznik odczytów rejestru msr
    unsigned char phase; // faza wykonywania rozkazu
    unsigned char data; // dana do zapisu / odczytu portu
    unsigned char buffer[16]; // bufor operacji
    unsigned char idx_0, idx_1; // zmienne licznikowe operacji na buforze
    struct i8272opc* opcode; // wskaźnik na instrukcję

    /* sprzęgnięcie z emulacją napędu */
    void* parameter[4]; // parametr wywołania funkcji napędu
    int (*drive)(  // funkcja obsługi napędu
        const void* i8272,   //      struktura FDC
        void* fdd,           //      parametr napędu (struktura opisu dyskietki)
        unsigned char action,//      kod operacji
        unsigned char wdata  //      dane do zapisu
    ); // zwracana wartość to dana odczytu lub kod błędu */

    /* sprzęg z CPU */
    unsigned char int_line;

    /* parametry dla operacji read/write */
    unsigned char rw; // flaga wskazująca na fazę exec komendy read lub write
    unsigned char cn; // numer cylindra
    unsigned char ha; // numer głowicy
    unsigned char rec; // numer sektora
    unsigned char nb; // ilość bajtów/sektor
    unsigned char eot; // koniec ścieżki
    unsigned char gpl; // długość GAP3
    unsigned char dtl; // ilość danych do zapisu/odczytu sektora, jeśli nb=0 */

    /* zmienne odczytu/zapisu sektora */
    unsigned int sbyte; // nr bajtu w sektorze, licznik
    unsigned int slen; // rozmiar sektora w bajtach
    unsigned int trlen; // rozmiar ścieżki w bajtach
    int err; // błąd napędu
};

/* Bity rejestru MSR */
enum {
    FDC_D0B = 0x01,
    FDC_D1B = 0x02,
    FDC_D2B = 0x04,
    FDC_D3B = 0x08,
    FDC_CB = 0x10,
    FDC_NDM = 0x20,
    FDC_DIO = 0x40,
    FDC_RQM = 0x80
};

/* fazy komend */
enum {
    FDC_COMMAND,
    FDC_EXEC,
    FDC_RESULT
};

/*
  Sprzęg z emulatorem
*/

/* zainicjowanie struktury */
void i8272_init(struct i8272* fdc);
/* ustwienie linii 'terminal count' */
void i8272_tc(struct i8272* fdc, unsigned char tc);
/* ustawienie linii 'reset' */
void i8272_reset(struct i8272* fdc, unsigned char reset);
/* zapis do rejestrów i8272, address_bit - A0  */
void i8272_out(struct i8272* fdc, unsigned char address_bit, unsigned char data);
/* odczyt z rejestrów i8272, address_bit - A0  */
unsigned char i8272_in(struct i8272* fdc, unsigned char address_bit);

/*
  Sprzęg z napędem wirtualnym, należy wywołać po i8272_init()
*/
void i8272_fdd_connect(
    struct i8272* fdc, /* struktura FDC */
    int (*drive)(const void*, void*, unsigned char, unsigned char), /* funkcja napędu wirtualnego */
    void*, void*, void*, void* /* struktury napędów 0,1,2,3 */
);

/* funkcje odpowiadające komendom i8272 */
#define i8272_CMD_R_DATA	(void (*)(void*))i8272_rd_cmd
#define i8272_CMD_R_DEL_DATA	(void (*)(void*))i8272_rdd_cmd
#define i8272_CMD_W_DATA	(void (*)(void*))i8272_wd_cmd
#define i8272_CMD_W_DEL_DATA	(void (*)(void*))i8272_wdd_cmd
#define i8272_CMD_R_TRACK	(void (*)(void*))i8272_rt_cmd
#define i8272_CMD_R_ID		(void (*)(void*))i8272_rid_cmd
#define i8272_CMD_F_TRACK	(void (*)(void*))i8272_ft_cmd
#define i8272_CMD_SCAN_EQ	(void (*)(void*))i8272_seq_cmd
#define i8272_CMD_SCAN_L_EQ	(void (*)(void*))i8272_sleq_cmd
#define i8272_CMD_SCAN_H_EQ	(void (*)(void*))i8272_sheq_cmd
#define i8272_CMD_RECALIBRATE	(void (*)(void*))i8272_rec_cmd
#define i8272_CMD_SENSE_IR_STAT (void (*)(void*))i8272_sis_cmd
#define i8272_CMD_SPECIFY	(void (*)(void*))i8272_specify_cmd
#define i8272_CMD_SENSE_DRV	(void (*)(void*))i8272_sds_cmd
#define i8272_CMD_SEEK		(void (*)(void*))i8272_seek_cmd
#define i8272_CMD_INVALID	(void (*)(void*))i8272_invalid_cmd

#define i8272_CMD_TABLE		{\
        { i8272_CMD_INVALID	 /* XXX 00000 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 00001 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_R_TRACK	 /* 0XX 00010 */, 9, 1, 7, "READ A TRACK" },\
        { i8272_CMD_SPECIFY	 /* 000 00011 */, 3, 0, 0, "SPECIFY" },\
        { i8272_CMD_SENSE_DRV	 /* 000 00100 */, 2, 0, 1, "SENSE A DRIVE" },\
        { i8272_CMD_W_DATA	 /* XX0 00101 */, 9, 1, 7, "WRITE DATA" },\
        { i8272_CMD_R_DATA	 /* XXX 00110 */, 9, 1, 7, "READ DATA" },\
        { i8272_CMD_RECALIBRATE	 /* 000 00111 */, 2, 1, 0, "RECALIBRATE" },\
        { i8272_CMD_SENSE_IR_STAT/* 000 01000 */, 1, 0, 2, "SENSE INTERRUPT STATUS" },\
        { i8272_CMD_W_DEL_DATA	 /* XX0 01001 */, 9, 1, 7, "WRITE DELETED DATA" },\
        { i8272_CMD_R_ID	 /* 0X0 01010 */, 2, 1, 7, "READ ID" },\
        { i8272_CMD_INVALID	 /* XXX 01011 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_R_DEL_DATA	 /* XXX 01100 */, 9, 1, 7, "READ DELETED DATA" },\
        { i8272_CMD_F_TRACK	 /* 0X0 01101 */, 6, 1, 7, "FORMAT A TRACK" },\
        { i8272_CMD_INVALID	 /* XXX 01110 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_SEEK	 /* 000 01111 */, 3, 1, 0, "SEEK" },\
        { i8272_CMD_INVALID	 /* XXX 10000 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_SCAN_EQ	 /* XXX 10001 */, 9, 1, 7, "SCAN EQUAL" },\
        { i8272_CMD_INVALID	 /* XXX 10010 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 10011 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 10100 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 10101 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 10110 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 10111 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 11000 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_SCAN_L_EQ	 /* XXX 11001 */, 9, 1, 7, "SCAN LOW OR EQUAL" },\
        { i8272_CMD_INVALID	 /* XXX 11010 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 11011 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 11100 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_SCAN_H_EQ	 /* XXX 11101 */, 9, 1, 7, "SCAN HIGH OR EQUAL" },\
        { i8272_CMD_INVALID	 /* XXX 11110 */, 1, 0, 1, "INVALID" },\
        { i8272_CMD_INVALID	 /* XXX 11111 */, 1, 0, 1, "INVALID" } \
    }

/* flagi dla st0 */
enum {
    FDC_IC = 0xC0,
    FDC_SE = 0x20,
    FDC_EC = 0x10,
    FDC_NR = 0x08,
    FDC_ST0_HD = 0x04,
    FDC_ST0_US1 = 0x02,
    FDC_ST0_US0 = 0x01
};

/* flagi dla st1 */
enum {
    FDC_EN = 0x80,
    FDC_DE = 0x20,
    FDC_OR = 0x10,
    FDC_ND = 0x04,
    FDC_NW = 0x02,
    FDC_MA = 0x01
};

/* flagi dla st2 */
enum {
    FDC_CM = 0x40,
    FDC_DD = 0x20,
    FDC_WC = 0x10,
    FDC_SH = 0x08,
    FDC_SN = 0x04,
    FDC_BC = 0x02,
    FDC_MD = 0x01
};

/* flagi dla st3 */
enum {
    FDC_FT = 0x80,
    FDC_WP = 0x40,
    FDC_RDY = 0x20,
    FDC_T0 = 0x10,
    FDC_TS = 0x08,
    FDC_ST3_HD = 0x04,
    FDC_ST3_US1 = 0x02,
    FDC_ST3_US0 = 0x01
};

/* flagi dla call */
enum {
    FDC_IN_MSR,
    FDC_IN_DATA_0,
    FDC_IN_DATA_1,
    FDC_OUT_DATA_0,
    FDC_OUT_DATA_1,
    FDC_CMD
};

void i8272_rd_cmd(struct i8272*);
void i8272_rdd_cmd(struct i8272*);
void i8272_wd_cmd(struct i8272*);
void i8272_wdd_cmd(struct i8272*);
void i8272_rt_cmd(struct i8272*);
void i8272_rid_cmd(struct i8272*);
void i8272_ft_cmd(struct i8272*);
void i8272_seq_cmd(struct i8272*);
void i8272_sleq_cmd(struct i8272*);
void i8272_sheq_cmd(struct i8272*);
void i8272_rec_cmd(struct i8272*);
void i8272_sis_cmd(struct i8272*);
void i8272_specify_cmd(struct i8272*);
void i8272_sds_cmd(struct i8272*);
void i8272_seek_cmd(struct i8272*);
void i8272_invalid_cmd(struct i8272*);

enum {
    // akcje FDD
    FDD_SEEK,
    FDD_READ_TRACK,
    FDD_WRITE_TRACK,
    FDD_SEEK_SECTOR,
    // Wartości zwracane przez FDD
    FDD_OK = 0,
    FDD_ERR_WPROT = 0x81,
    FDD_ERR_NODISK = 0x82,
    FDD_ERR_OVER = 0x83
};

/* kody przerwania dla rejestru ST0 */
enum {
    FDC_IC_NT = 0, /* Normalne zakończenie. Rozkaz kompletny i poprawnie wykonany. */
    FDC_IC_AT0 = 1, /* Błąd wykonania rozkazu. Wykonanie rozkazu rozpoczęte, ale nie zakończone */
    FDC_IC_IC = 2, /* Błądny rozkaz. */
    FDC_IC_AT1 = 3 /* Błąd wykonania rozkazu ponieważ, w czasie jego wykonania zmianie uległa linia ready */
};