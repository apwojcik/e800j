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

#ifndef __e800j_h__
#define __e800j_h__

#include "z80.h"
#include "i8251.h"

typedef struct
{
    byte *screen[2];
    byte blink:1;
    byte :7;

} tv_screen;

typedef struct
{
    /* zawartosci pamiêci sta³ych */
    byte *rom_bas0;		/* ROM BAS0 E800 Junior 					*/
    byte *rom_bas1;		/* ROM BAS1 E800 Junior 					*/
    byte *rom_boot;		/* ROM BOOT E800 Junior 					*/
    byte *prom_mem;		/* PROM MEM1            					*/
    byte *prom_jio;		/* PROM J/O1            					*/
    byte *eprom_tv;		/* EPROM TV             					*/
    word addr_prom_mask;	/* maska z dodatkowymi 3 bitami sterowania bankami ROM 		*/
    byte *ram;			/* 64kB RAM             					*/

    /* rejestry  */
    byte reg175;		/* warto¶æ górnych 4 bitów rejestru konfiguracji pamiêci    	*/
    byte reg373;		/* OUT #FE - kolor ramki, wyjscie na magnetofon, glosniczek 	*/
    
    /* flagi operacji na  rejestrach */
    byte wr175:1;		/* nastapi³ zapis do rejestru 74LS175 - konfiguracja pamiêci	*/
    byte wr373:1;    		/* nastapi³ zapis do rejestru 74LS373 - port wyj¶ciowy #FE	*/
    byte wr55:1;		/* nast±pi³ zapis do portu 8255	- LOGABAX			*/
    byte wr51:1;		/* nast±pi³ zapis do portu 8251	- JUNET				*/
    byte wr72:1;		/* nast±pi³ zapis do portu 8272	- kontroler FDD			*/
    byte wrFDC:1;		/* nast±pi³ zapis do karty FDC na porcie #F1			*/

    /* jednobitowe linie wejscia/wyjscia */
    byte tape_in:1;		/* wejscie  magnetofonu / sieci spektrum			*/
    byte tape_out:1;		/* wyjscie  magnetofonu						*/
    byte spkr_out:1;		/* wyjscia  glosnika						*/

/* inne flagi */
    byte :7;    

    /* struktura sterowania ekranem */        
    tv_screen	scr;		/* pamieæ ekranu, atrybuty itp					*/

    /* równoleg³y port wej¶cia/wyj¶cia  */
//    _8255	i8255;		/* drukarka (standard LOGABAX), manipulator (np KEMSTON JSTK)	*/

    /* szeregowy port wejscia/wyjscia   */
    i8251	*i8251p;	/* sieæ JUNET							*/

    /* karta FDC		*/
//    str_fdc	fdc;		/* kontroler stacji dysków					*/
} e800j;


#define COL_NRM  0xb000
#define COL_BRT  0xf000
#define COL_WHT	 0xe000
#define ZX_PALETTE	{   {0x0000,0x0000 ,0x0000 ,0x0000 },\
			    {0x0000,0x0000 ,0x0000 ,COL_NRM},\
			    {0x0000,COL_NRM,0x0000 ,0x0000 },\
			    {0x0000,COL_NRM,0x0000 ,COL_NRM},\
			    {0x0000,0x0000 ,COL_NRM,0x0000 },\
			    {0x0000,0x0000 ,COL_NRM,COL_NRM},\
			    {0x0000,COL_NRM,COL_NRM,0x0000 },\
			    {0x0000,COL_WHT,COL_WHT,COL_WHT},\
							     \
    			    {0x0000,0x0000 ,0x0000 ,0x0000 },\
			    {0x0000,0x0000 ,0x0000 ,COL_BRT},\
			    {0x0000,COL_BRT,0x0000 ,0x0000 },\
			    {0x0000,COL_BRT,0x0000 ,COL_BRT},\
			    {0x0000,0x0000 ,COL_BRT,0x0000 },\
			    {0x0000,0x0000 ,COL_BRT,COL_BRT},\
			    {0x0000,COL_BRT,COL_BRT,0x0000 },\
			    {0x0000,COL_BRT,COL_BRT,COL_BRT}}


/* testy bitów rejestru konfiguracji pamiêci 74LS157 */
#define MEM157_JUNET(r)	(r & 0x10)	/* czy ustawiony bit odczytu adresu w sieci   	*/
#define MEM157_RELOK(r)	(r & 0x20)	/* czy ustawiony bit relokacji pamiêci ekranu 	*/
#define MEM157_BOOT(r)	(r & 0x40)	/* sprzêtowe maskowanie NMI, od 8251          	*/
#define MEM157_BIT7(r)	(r & 0x80) 	/* ósmy bit rejestru 				*/
#define MEM157_BIT67(r)	(r & 0xc0) 	/* ósmy i siódmy bit rejestru 			*/
#define MEM157_PROM(r)	((MEM157_BIT67(r) >> 1) | (MEM157_RELOK(r) << 3)) /* maska dla adresu prom 	*/

#define Z80E800ROM(zi)	((e800j *)zi->cust)

#define MEM_ROM1(a)	(!(a & 0x01))	/* rom1 'BAS0' za³±czony */
#define MEM_ROM2(a)	(!(a & 0x10))	/* rom2 'BOOT' za³±czony */
#define MEM_ROM3(a)	(!(a & 0x02))	/* rom3 'BAS1' za³±czony */
#define MEM_STAT(a)	(!(a & 0x04))	/* zapis do bufora atrybutów ??*/
#define MEM_DRAM(a)	(!(a & 0x08))	/* odczyt z DRAM         */
#define MEM_IAHR(a)	(!(a & 0x20))	/* RST #18               */
#define ROM_ADDR(a)	(a & 0x1fff)	/* maska adresu          */

#define IO_CFE(a)	(!( a & 0x01))	/* klawiatura           */
#define IO_CF7(a)	(!( a & 0x02))  /* konfiguracja pamiêci */
#define IO_CS55(a)	(!( a & 0x04))  /* konfiguracja pamiêci */
#define IO_CSFDC(a)	(!( a & 0x08))  /* sterowanie FDC       */
#define IO_CS51(a)	(!( a & 0x10))  /* port szeregowy 8251  */
#define IO_CF1(a)	(!( a & 0x20))  /* ???                  */
#define IO_IAL(a)	(!( a & 0x40))  /* RST #18              */

#define A0_A8(a)	(a & 0x1ff)
#define RST_18		0x18
#define TEST_A9(a)	(a & 0x200)

#define E800J(a)	((e800j *)a)
#define A0(a)		(a & 0x01)
#define A0(a)		(a & 0x01)
#define A1_A0(a)	(a & 0x03)
#define A9(a)		(a & 0x0200)
#endif













