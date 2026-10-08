/* Elwro 800 Junior emulator
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

#ifndef __i8251_h__
#define __i8251_h__

typedef struct 
{
    /* blok sterowania */
    unsigned char mode;		/* s這wo trybu			*/
    unsigned char sync_1;	/* s這wo synchronizacji I	*/
    unsigned char sync_2;	/* s這wo synchronizacji II	*/
    unsigned char instr;	/* s這wo rozkazu		*/
    unsigned char status;	/* s這wo stanu                  */

    /* bufor linii danych */    
    unsigned char TxD[2];	/* bufor danych wyj軼iowych	*/
    unsigned char RxD[2];	/* bufor danych wej軼iowych	*/

    /* wewnetrzne */    
    char instr_cnt;		/* liczy kolejne bajty sterowania */
    int txd_cnt;		/* liczy ilo嗆 s堯w w buforze TX */
} i8251;

/* rejestr statusu */
#define STAT_TXRDY	(chip->status & 0x01)
#define STAT_SET_TXRDY	chip->status &= ~0x01
#define STAT_CLR_TXRDY	chip->status |= 0x01
#define STAT_RXRDY	(chip->status & 0x02)
#define STAT_SET_RXRDY	chip->status |= 0x02
#define STAT_CLR_RXRDY	chip->status &= ~0x02
#define STAT_TXE	(chip->status & 0x04)
#define STAT_SET_TXE	chip->status |= 0x04
#define STAT_CLR_TXE	chip->status &= ~0x04
#define STAT_PE		(chip->status & 0x08)
#define STAT_SET_PE	chip->status |= 0x08
#define STAT_CLR_PE	chip->status &= ~0x08
#define STAT_OE		(chip->status & 0x10)
#define STAT_SET_OE	chip->status |= 0x10
#define STAT_CLR_OE	chip->status &= ~0x10
#define STAT_FE		(chip->status & 0x20)
#define STAT_SET_FE	chip->status |= 0x20
#define STAT_CLR_FE	chip->status &= ~0x20
#define STAT_SYNDET	(chip->status & 0x40)
#define STAT_SET_SYNDET	chip->status |= 0x40
#define STAT_CLR_SYNDET	chip->status &= ~0x40
#define STAT_DSR	(chip->status & 0x80)
#define STAT_SET_DSR	chip->status |= 0x80
#define STAT_CLR_DSR	chip->status &= ~0x80

/* rejestr rozkazu */
#define INSTR_TXEN	(chip->instr & 0x01)
#define INSTR_SET_TXEN	chip->instr &= ~0x01
#define INSTR_CLR_TXEN	chip->instr |= 0x01
#define INSTR_DTR	(chip->instr & 0x02)
#define INSTR_SET_DTR	chip->instr |= 0x02
#define INSTR_CLR_DTR	chip->instr &= ~0x02
#define INSTR_RXEN	(chip->instr & 0x04)
#define INSTR_SET_RXEN	chip->instr |= 0x04
#define INSTR_CLR_RXEN	chip->instr &= ~0x04
#define INSTR_SBRK	(chip->instr & 0x08)
#define INSTR_SET_SBRK	chip->instr |= 0x08
#define INSTR_CLR_SBRK	chip->instr &= ~0x08
#define INSTR_ER	(chip->instr & 0x10)
#define INSTR_SET_ER	chip->instr |= 0x10
#define INSTR_CLR_ER	chip->instr &= ~0x10
#define INSTR_RTS	(chip->instr & 0x20)
#define INSTR_SET_RTS	chip->instr |= 0x20
#define INSTR_CLR_RTS	chip->instr &= ~0x20
#define INSTR_IR	(chip->instr & 0x40)
#define INSTR_SET_IR	chip->instr |= 0x40
#define INSTR_CLR_IR	chip->instr &= ~0x40
#define INSTR_EH	(chip->instr & 0x80)
#define INSTR_SET_EH	chip->instr |= 0x80
#define INSTR_CLR_EH	chip->instr &= ~0x80

/* rejestr trybu */
#define MODE_DIV	(chip->mode & 0x03)	/* Tryb transferu / dzielnik f txc,rxc  */
#define MODE_WL		(chip->mode & 0x0c)	/* dlugosc slowa 			*/
#define MODE_PEN	(chip->mode & 0x10)	/* kontrola parzystosci 		*/
#define MODE_EP		(chip->mode & 0x20)	/* spos鏏 kontroli parz/nieparz 	*/
/* dla trybu synchronicznego */
#define MODE_ESD	(chip->mode & 0x40)	/* kierunek linii syndet		*/
#define MODE_SCS	(chip->mode & 0x80)	/* liczba slow synchro			*/
/* dla trybu asynchronicznego */
#define MODE_STOP	(chip->mode & 0x40)	/* liczba bit闚 stopu 			*/

/** Strona CPU *******************************/
void i8251_reset(i8251 *chip);
void i8251_write(i8251 *chip,char data,char cd);
unsigned char i8251_read(i8251 *chip,char cd);

/********* Strona ukladu dla urz康zenia zewn皻rnego  *********/
unsigned char i8251_txd(i8251 *chip);
void i8251_rxd(i8251 *chip, char data);
unsigned char i8251_status(i8251 *chip);

#endif
