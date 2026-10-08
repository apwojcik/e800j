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

#ifndef  __z80_debug_h__
#define  __z80_debug_h__

#include "z80.h"



typedef struct{
    byte lines; /* ilosc linii instrukcji jaka ma zostac wyswietlona */
    long  instr; /* ilosc instrukcji do wykonania */
    word addr_brk; /* pulapka na adresu równy */
    word addr_ghe; /* pulapka na adres wiêkszy równy  */
    word addr_gle; /* pulapka na adres mniejszy równy */
    byte fwcnt:1;
    byte fcnt:1;
    byte fbreak:1;
    byte fwadr:1;
    byte free:1;
    byte fprint:1;
    byte hdr:1;
    byte pout;
    byte stdout;
    word daddr;
    word deasm;
    
    int call_d;
    byte djnz_d:1;
    byte debug_print:1;
    byte init:1;
    byte fin:1;			/* zmienna, która gdy jest =1 nakazuje deasemblacjê na koncu kroku */
    byte proc:1;
    byte deb_stdout:1;
    long cnt;
    void (*input)(char *);
    void (*output)(char *);
    void (*exit)(void);

} Z80_DEB;

#define DEB(a)	((Z80_DEB*)(a)->deb_struct)

/**/
#define DEB_CTL_RESET	0
#define DEB_CTL_DEASM	1
#define DEB_CTL_DEBUG	2
#define DEBUGGER_RESET(z)	z80_deb_call(z,DEB_CTL_RESET)

extern void z80_deb_call(void *z, char call);
extern void z80_debugger_init(Z80 *z, Z80_DEB *deb, void (*deb_in)(char*), void (*deb_out)(char*), void (*exit_f)(void));
extern void z80_set_debug(Z80 *z,char fl);
extern char z80_debug_test_break(Z80 *);

#endif
