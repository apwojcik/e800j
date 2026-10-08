/* uPD765/i8272 Floppy Disc Controller emulation ( NO DMA MODE )
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i8272.h"

#define DINFO(a)	printf(a);
//#define DINFO(a)

#define I8272		ct = ct ? ct : p_i8272_
#define I8272M(a)	((i8272 *)a)

/* FDC command */
#define READ_TRACK	0x02
#define SPECIFY		0x03
#define SENSE_DRV_STAT	0x04
#define WRITE_DATA	0x05
#define READ_DATA	0x06
#define RECALIBRATE	0x07
#define SENSE_INT_STAT	0x08
#define WRITE_DEL_DATA	0x09
#define READ_ID		0x0a
#define READ_DEL_DATA	0x0c
#define FORMAT_TRACK	0x0d
#define SEEK		0x0f
#define SCAN_EQ		0x11
#define SCAN_LE		0x19
#define SCAN_HE		0x1d




/* zakoñcz instrukcjê, i przejdz do fazy RESULT */
#define SET_RESULT(a) I8272M(a)->phase = PH_RESULT
#define DISK_SEL(a) (I8272M(a)->fifo[1] & 3)

void i8272_ck_command(i8272 *ct);
void i8272_ck_execute(i8272 *ct);
void i8272_ck_result(i8272 *ct);
void i8272_read_data(void *ct);
void i8272_read_del_data(void *ct);
void i8272_write_data(void *ct);
void i8272_write_del_data(void *ct);
void i8272_read_track(void *ct);
void i8272_read_id(void *ct);
void i8272_format_track(void *ct);
void i8272_scan_eq(void *ct);
void i8272_scan_le(void *ct);
void i8272_scan_he(void *ct);
void i8272_recalibrate(void *ct);
void i8272_sense_int_stat(void *ct);
void i8272_specify(void *ct);
void i8272_sense_drv_stat(void *ct);
void i8272_seek(void *ct);
void i8272_invalid(void *ct);

i8272 _i8272_;			/* domyslna struktura i8272 */
i8272 *p_i8272_ = &_i8272_;

/* zakancza kazda komende w fazie result  */
void i8272_command_tc(i8272 *ct)
{
    ct->msr |= MSR_RQM; 
    ct->msr &= ~MSR_DIO;
    ct->msr &= ~MSR_CB;
    ct->phase = PH_COMMAND;
    ct->cm_ct = 0;
    ct->cmd = NULL;    
printf("-----------------------------\n");
}

void i8272_push_fifo(i8272 *ct, usc val)
{
    if(ct->sp >= 16){
	return;
    } /* proba wyslania do pelnego FIFO  */
    ct->fifo[ct->sp++] = val;
}

usc i8272_pop_fifo(i8272 *ct)
{
    if(ct->sp == 0){
	return 0xff;
    } /* czytanie z pustego FIFO */
    if(! --ct->sp ) ct->msr &= ~MSR_DIO;
    return ct->fifo[ct->sp];
}

void _i8272_ini_(i8272 *ct,void *fdd_0,void *fdd_1,void *fdd_2,void *fdd_3, void *irq)
{
    I8272;
    ct->fdd0 = fdd_0;
    ct->fdd1 = fdd_1;
    ct->fdd2 = fdd_2;
    ct->fdd3 = fdd_3;
    ct->irq  = irq;
    _i8272_rst_(ct);
}

void _i8272_rst_(i8272 *ct)
{
    void *a,*b,*c,*d,*e;
    usi	 i;
    I8272;
    DINFO(">>>>> FDC RESET <<<<<\n");
    a = ct->fdd0;
    b = ct->fdd1;
    c = ct->fdd2;
    d = ct->fdd3;
    e = ct->irq;    

    for(i = 0; i < sizeof(i8272); i++) *((usc *)ct + i) = 0x00;
    
    ct->fdd0 = a;
    ct->fdd1 = b;
    ct->fdd2 = c;
    ct->fdd3 = d;
    ct->irq  = e;    
    ct->st_0 = 0x80;
    ct->msr  = 0x80;
}


void _i8272_tc_(i8272 *ct)
{
    I8272;
    i8272_command_tc(ct);
    ct->st_0 |= ST0_IC_0;
}

void _i8272_wr_(i8272 *ct, usi addr, usc val)
{
    I8272;
    if( !( addr & 1) || !(ct->msr & MSR_RQM) || (ct->msr & MSR_DIO) ) return;
    ct->msr &= ~MSR_RQM;
    i8272_push_fifo(ct,val);
}

usc  _i8272_rd_(i8272 *ct, usi addr)
{
    I8272;
    if(!(addr & 1)) return ct->msr;
    return i8272_pop_fifo(ct);
}

/***************************************************************************************************
 *
 * Dekodowanie instrukcji, rozdzia³ wykonania rozkazu, sterowanie msr
 *
 */
void _i8272_ck_(i8272 *ct)
{
    I8272;

    switch(ct->phase)
    {
	case PH_COMMAND : i8272_ck_command(ct); break;
 	case PH_EXEC    : i8272_ck_execute(ct); break;
	case PH_RESULT  : i8272_ck_result(ct); break;
    }
}


void i8272_ck_command(i8272 *ct)
{
    if(!ct->sp) return;	/* jesli FIFO puste pomin  */
    if((ct->sp == 1) && !(ct->msr & MSR_CB)){ /* bajt rozkazu do dekodowania */
	ct->msr |= MSR_CB;
	if((ct->fifo[0] & 0x1f) == SENSE_INT_STAT) ct->sis_force = 0x00;
	switch((ct->fifo[0] & 0x1f) | ct->sis_force){ /* get first fifo byte, and decode instruction */
	    case READ_DATA	:ct->cm_ct = 0x9; ct->cmd = i8272_read_data; break;
	    case READ_DEL_DATA	:ct->cm_ct = 0x9; ct->cmd = i8272_read_del_data; break;
	    case WRITE_DATA	:ct->cm_ct = 0x9; ct->cmd = i8272_write_data; break;
	    case WRITE_DEL_DATA	:ct->cm_ct = 0x9; ct->cmd = i8272_write_del_data; break;
	    case READ_TRACK	:ct->cm_ct = 0x9; ct->cmd = i8272_read_track; break;
	    case READ_ID	:ct->cm_ct = 0x2; ct->cmd = i8272_read_id; break;
	    case FORMAT_TRACK	:ct->cm_ct = 0x6; ct->cmd = i8272_format_track; break;
	    case SCAN_EQ	:ct->cm_ct = 0x9; ct->cmd = i8272_scan_eq; break;
	    case SCAN_LE	:ct->cm_ct = 0x9; ct->cmd = i8272_scan_le; break;
	    case SCAN_HE	:ct->cm_ct = 0x9; ct->cmd = i8272_scan_he; break;
	    case RECALIBRATE	:ct->cm_ct = 0x2; ct->sis_force = 0x00;ct->cmd = i8272_recalibrate; break;
	    case SENSE_INT_STAT :ct->cm_ct = 0x1; ct->sis_force = 0x00;ct->cmd = i8272_sense_int_stat; break;
	    case SPECIFY	:ct->cm_ct = 0x3; ct->cmd = i8272_specify; break;
	    case SENSE_DRV_STAT	:ct->cm_ct = 0x2; ct->cmd = i8272_sense_drv_stat; break;
	    case SEEK		:ct->cm_ct = 0x3; ct->sis_force = 0x00;ct->cmd = i8272_seek; break;
	    default		:ct->cm_ct = 0x1; ct->sis_force = 0x00;ct->cmd = i8272_invalid; break;
	}    
    }    
    if(ct->sp == ct->cm_ct){ /* rozkaz skompletowany, przejscie do fazy wykonania */
//static zz=0;
	ct->cm_ct = 0;
	ct->sp = 0;	
	ct->phase = PH_EXEC;	
	ct->msr &= ~MSR_RQM;
	ct->msr |= MSR_DIO;

	if((ct->fifo[0] & 0x1f) == RECALIBRATE){
//	     ct->msr &= ~MSR_CB;
//	     zz++;
	}

//	if((ct->fifo[0] & 0x1f) == SEEK){
//	     ct->msr &= ~MSR_CB;
//	     zz=0;
//	}

/*
if(((ct->fifo[0] & 0x1f) == SENSE_INT_STAT) && zz){
     ct->msr &= ~MSR_CB;
    zz=0;
}
*/
	return;
    }
    ct->msr |= MSR_RQM; /* gotowy do odbioru nastepnej danej */
}

void i8272_ck_execute(i8272 *ct)
{
    if(ct->cmd) ct->cmd(ct);		/* wykonanie instrukcji w fazie result */
}

void i8272_ck_result(i8272 *ct)
{
    ct->msr |= MSR_RQM;			/* ustawia RQM - zezwolenie na odczyt danych w fazie result */
    ct->msr &= ~MSR_DIO;
    if(!ct->sp){
	  i8272_command_tc(ct); 	/* zakoñcz instrukcjê i przejdz do fazy COMMAND 	    */
    }
    else{
      ct->msr |= MSR_DIO;
    }
}	

/***************************************************************************************************
*
*
* FAZA EXECUTE - wykonanie instrukcji
*
*/
void i8272_invalid(void *ct)
{
    DINFO(">>>>> FDC COMMAND: INVALID <<<<<\n");
    I8272M(ct)->st_0 = 0x80;
//    I8272M(ct)->sis_force = 0x00;    
    i8272_push_fifo(I8272M(ct),0x80);
    SET_RESULT(ct);
}

void i8272_read_data(void *ct)
{
    DINFO(">>>>> FDC COMMAND: READ DATA <<<<<\n");
exit(0);    
}

void i8272_read_del_data(void *ct)
{
    DINFO(">>>>> FDC COMMAND: READ DELETED DATA <<<<<\n");
exit(0);    
}

void i8272_write_data(void *ct)
{
    DINFO(">>>>> FDC COMMAND: WRITE DATA <<<<<\n");
exit(0);    
}

void i8272_write_del_data(void *ct)
{
    DINFO(">>>>> FDC COMMAND: WRITE DELETED DATA <<<<<\n");
exit(0);    
}

void i8272_read_track(void *ct)
{
    DINFO(">>>>> FDC COMMAND: READ TRACK <<<<<\n");
exit(0);    
}

void i8272_read_id(void *ct)
{
    DINFO(">>>>> FDC COMMAND: READ ID <<<<<\n");
exit(0);    
}

void i8272_format_track(void *ct)
{
    DINFO(">>>>> FDC COMMAND: FORMAT TRACK <<<<<\n");
exit(0);    
}

void i8272_scan_eq(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SCAN EQUAL <<<<<\n");
exit(0);    
}

void i8272_scan_le(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SCAN LESS OR EQUAL <<<<<\n");
exit(0);    
}

void i8272_scan_he(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SCAN HIGH OR EQUAL <<<<<\n");
exit(0);    
}

void i8272_recalibrate(void *ct)
{
    DINFO(">>>>> FDC COMMAND: RECALIBRATE <<<<<\n");
    I8272M(ct)->cn[DISK_SEL(ct)] = 0;
    I8272M(ct)->pcn = 0;
    I8272M(ct)->st_0 |= ST0_SE;
    I8272M(ct)->st_0 &= ~ST0_IC_1;
    I8272M(ct)->st_0 &= ~ST0_IC_0;
I8272M(ct)->st_0 = 0xc0;
//    I8272M(ct)->msr |= MSR_CB;

    SET_RESULT(ct);
}

void i8272_sense_int_stat(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SENSE INTERRUPT STATUS <<<<<\n");

    i8272_push_fifo(I8272M(ct),I8272M(ct)->pcn);
    i8272_push_fifo(I8272M(ct),I8272M(ct)->st_0);
    SET_RESULT(ct);
//I8272M(ct)->st_0 = 0x80;
//I8272M(ct)->msr &= ~MSR_CB;
/*
{
    static int i=0;
    i++;
    if(i == 6) exit(0);
}
*/
}

void i8272_specify(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SPECIFY <<<<<\n");
    I8272M(ct)->srt_hut = I8272M(ct)->fifo[1];
    I8272M(ct)->hlt_nd  = I8272M(ct)->fifo[2];    
    I8272M(ct)->msr =  (I8272M(ct)->hlt_nd & 1) ? I8272M(ct)->msr | MSR_ND : I8272M(ct)->msr & ~MSR_ND;
    SET_RESULT(ct);
}

void i8272_sense_drv_stat(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SENSE DRIVE STATUS <<<<<\n");
exit(0);    
}

void i8272_seek(void *ct)
{
    DINFO(">>>>> FDC COMMAND: SEEK <<<<<\n");
    I8272M(ct)->hds[ DISK_SEL(ct)] = (I8272M(ct)->fifo[1] >> 2) & 1;
    I8272M(ct)->cn[DISK_SEL(ct)] = I8272M(ct)->fifo[2];

    I8272M(ct)->st_0 |= ST0_SE;
    I8272M(ct)->st_0 &= ~ST0_IC_1;
    I8272M(ct)->st_0 &= ~ST0_IC_0;

    SET_RESULT(ct);
}



