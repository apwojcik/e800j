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

#include "i8251.h"

void i8251_reset(i8251 *chip)
{
    char i;

    for(i=0; i < sizeof(i8251); i++) *((char *)chip+i) = 0;
    STAT_SET_TXE; 	/* bufor pusty 				*/
    STAT_SET_TXRDY;	/* gotowosc przyjecia slowa do wyslania */
}

void i8251_write(i8251 *chip,char data,char cd)
{
    if(cd){
	switch(chip->instr_cnt){
	    case 0: chip->mode = data; 
		    chip->instr_cnt = MODE_DIV ? 3 : 1; 
		    return;
	    case 1: chip->sync_1 = data;
		    chip->instr_cnt = MODE_SCS ? 3 : 2;
		    return;
	    case 2: chip->sync_2 = data;
		    chip->instr_cnt=3;
		    return;
	    case 3: chip->instr = data;
		    chip->instr_cnt=0;
		    if(INSTR_IR) i8251_reset(chip);
		    if(INSTR_ER) {STAT_CLR_PE;STAT_CLR_OE;STAT_CLR_FE;};
		    return;
	}
    } else {
	if(!INSTR_TXEN) return;
	if(chip->txd_cnt > 1) return;	 /* nastepne slowa sa ignorowane dopuki sie bufor nie opró¿ni */
	chip->TxD[chip->txd_cnt] = data;
	STAT_CLR_TXE; 			/* bufor ju¿ nie jest pusty */	
	if(chip->txd_cnt) STAT_CLR_TXRDY;
	chip->txd_cnt++;		
    }
}

unsigned char i8251_read(i8251 *chip,char cd)
{
    if(cd) return chip->status;
    STAT_CLR_RXRDY;
    return chip->RxD[0];
}

unsigned char i8251_txd(i8251 *chip)
{
    unsigned char tmp = chip->TxD[0];

    if(INSTR_SBRK) return 0;
    chip->TxD[0] = chip->TxD[1];
    STAT_SET_TXRDY;
    if(chip->txd_cnt) chip->txd_cnt--; else STAT_SET_TXE;
    return tmp;
}

void i8251_rxd(i8251 *chip, char data)
{
    if(!INSTR_RXEN) return;
    chip->RxD[0] = data;
    if(STAT_RXRDY) STAT_SET_OE;
    STAT_SET_RXRDY;
}

unsigned char i8251_status(i8251 *chip)
{
    return chip->status;
}

