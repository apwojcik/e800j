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

#ifndef _i8272_fdc_h_
#define _i8272_fdc_h_

#ifndef uchar
#define uchar unsigned char
#endif

typedef struct
{
    void (*command)(void *);	/* funkcja obsługi          */
    uchar cmd_len;		/* ilość bajtów rozkazu     */
    uchar exe_len;		/* faza execute ?           */
    uchar res_len;		/* ilość bajtów fazy result */
    char *name;
} i8272opc;

typedef struct
{
/* rejestry układu i8272A */
    uchar msr;		/* main status register */
    uchar st0;		/* rejestr statusowy 0  */
    uchar st1;		/* rejestr statusowy 1  */
    uchar st2;		/* rejestr statusowy 2  */
    uchar st3;		/* rejestr statusowy 3  */

/* parametry wewn FDC */
    uchar srt;		/* "Step Rate Time"      */
    uchar hut;		/* "Head Unload Time "   */
    uchar hlt;		/* "Head Load Time"      */
    uchar hds;		/* "HeaD Select"         */
    uchar pcn;    	/* "Present Cylinder Number" */
    uchar ds;		/* "Drive Select"        */

/* zmienne interpretacji rozkazu */
    uchar call;		/* miejsce wywołania opcode               */
    uchar icntr;	/* licznik odczytów rejestru msr          */
    uchar phase;	/* faza wykonywania rozkazu               */
    uchar data;		/* dana do zapisu / odczytu portu         */
    uchar buffer[16];	/* bufor operacji                         */
    uchar idx_0, idx_1; /* zmienne licznikowe operacji na buforze */
    i8272opc *opcode;	/* wskaźnik na instrukcję                 */

/* sprzęgnięcie z emulacją napędu */
    void  *parameter[4];/* parametr wywołania funkcji napędu  */
    int (*drive)( /* funkcja obsługi napędu */
            void *i8272, 	/* struktura FDC   */
            void *fdd, 	/* parametr napędu (struktura opisu dyskietki )*/
            uchar action, 	/* kod operacji   */
            uchar wdata	/* dane do zapisu */
        ); /* zwracana wartość to dana odczytu lub kod błędu */

/* sprzęg z CPU */
    uchar int_line;

/* parametry dla operacji read/write */
    uchar rw;		/* flaga wskazująca na fazę exec komendy read lub write */
    uchar cn;		/* numer cylindra      */
    uchar ha;		/* numer głowicy       */
    uchar rec;		/* numer sektora       */
    uchar nb;		/* ilość bajtów/sektor */
    uchar eot;		/* koniec ścieżki      */
    uchar gpl;		/* długość GAP3        */
    uchar dtl;		/* ilość danych do zapisu/odczytu sektora, jeśli nb=0 */
/* zmieene odczytu/zapisu sektora */
    unsigned int sbyte;  /* nr bajtu w sektorze, licznik   */
    unsigned int slen;   /* rozmiar sektora w bajtach      */
    unsigned int trlen;  /* rozmiar ścieżki w bajtach      */
    int err;	/* błąd napędu */
} i8272;

/* Bity rejestru MSR */
enum
{
    FDC_D0B = 0x01,
    FDC_D1B = 0x02,
    FDC_D2B = 0x04,
    FDC_D3B = 0x08,
    FDC_CB  = 0x10,
    FDC_NDM = 0x20,
    FDC_DIO = 0x40,
    FDC_RQM = 0x80
};

/* fazy komend */
enum
{
    FDC_COMMAND,
    FDC_EXEC,
    FDC_RESULT
};

/*
  Sprzęg z emulatorem
*/
/* zainicjowanie struktury */
extern void i8272_init(i8272 *fdc);
/* ustwienie linii 'terminal count' */
extern void i8272_tc(i8272 *fdc, unsigned char tc);
/* ustawienie linii 'reset' */
extern void i8272_reset(i8272 *fdc, unsigned char reset);
/* zapis do rejestrów i8272, address_bit - A0  */
extern void i8272_out(i8272 *fdc, unsigned char address_bit, unsigned char data);
/* odczyt z rejestrów i8272, address_bit - A0  */
extern unsigned char i8272_in(i8272 *fdc, unsigned char address_bit);

/*
  Sprzęg z napędem wirtualnym, należy wowołać po i8272_init()
*/
extern void i8272_fdd_connect(
    i8272 *fdc, 				/* struktura FDC */
    int (*drive)(void *, void *, uchar, uchar), /* funkcja napędu wirtualnego */
    void *, void *, void *, void*               /* struktury napędów 0,1,2,3 */
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
enum{
    FDC_IC  = 0xC0,
    FDC_SE  = 0x20,
    FDC_EC  = 0x10,
    FDC_NR  = 0x08,
    FDC_ST0_HD  = 0x04,
    FDC_ST0_US1 = 0x02,
    FDC_ST0_US0 = 0x01
};

/* flagi dla st1 */
enum{
    FDC_EN = 0x80,
    FDC_DE = 0x20,
    FDC_OR = 0x10,
    FDC_ND = 0x04,
    FDC_NW = 0x02,
    FDC_MA = 0x01
};

/* flagi dla st2 */
enum{
    FDC_CM = 0x40,
    FDC_DD = 0x20,
    FDC_WC = 0x10,
    FDC_SH = 0x08,
    FDC_SN = 0x04,
    FDC_BC = 0x02,
    FDC_MD = 0x01
};

/* flagi dla st3 */
enum{
    FDC_FT  = 0x80,
    FDC_WP  = 0x40,
    FDC_RDY = 0x20,
    FDC_T0  = 0x10,
    FDC_TS  = 0x08,
    FDC_ST3_HD  = 0x04,
    FDC_ST3_US1 = 0x02,
    FDC_ST3_US0 = 0x01
};

/* flagi dla call */
enum{
    FDC_IN_MSR,
    FDC_IN_DATA_0,
    FDC_IN_DATA_1,
    FDC_OUT_DATA_0,
    FDC_OUT_DATA_1,
    FDC_CMD
};

void i8272_rd_cmd(i8272*);
void i8272_rdd_cmd(i8272*);
void i8272_wd_cmd(i8272*);
void i8272_wdd_cmd(i8272*);
void i8272_rt_cmd(i8272*);
void i8272_rid_cmd(i8272*);
void i8272_ft_cmd(i8272*);
void i8272_seq_cmd(i8272*);
void i8272_sleq_cmd(i8272*);
void i8272_sheq_cmd(i8272*);
void i8272_rec_cmd(i8272*);
void i8272_sis_cmd(i8272*);
void i8272_specify_cmd(i8272*);
void i8272_sds_cmd(i8272*);
void i8272_seek_cmd(i8272*);
void i8272_invalid_cmd(i8272*);

enum{
/* akcje FDD */
    FDD_SEEK,
    FDD_READ_TRACK,
    FDD_WRITE_TRACK,
    FDD_SEEK_SECTOR,
/* Wartości zwracane przez FDD */
    FDD_ERR_WPROT  = -1,
    FDD_ERR_NODISK = -2,
    FDD_ERR_OVER = -3,
    FDD_OK = 0,
};

/* kody przerwania dla rejestru ST0 */
enum{
    FDC_IC_NT  = 0,	/* Normalne zakończenie. Rozkaz kompletny i poprawnie wykonany. */
    FDC_IC_AT0 = 1,	/* Błąd wykonania rozkazu. Wykonanie rozkazu rozpoczęte, ale nie zakończone */
    FDC_IC_IC  = 2,	/* Błędny rozkaz. */
    FDC_IC_AT1 = 3	/* Błąd wykonania rozkazu ponieważ, w czasie jego wykonania zmianie uległa linia ready */
};

#endif


