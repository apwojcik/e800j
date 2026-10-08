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

#ifndef __i8272_h__
#define __i8272_h__
#define usc unsigned char
#define usi unsigned short int

typedef struct 
{
    long	timer;
/* status registers */
    usc		msr;		/* main status register		*/
    usc		st_0;		/* status register 0		*/
    usc		st_1;		/* status register 1		*/
    usc		st_2;		/* status register 2		*/
    usc		st_3;		/* status register 3		*/
/* FIFO */
    usc		fifo[16];	/* input/output fifo data buffer */
    usi		sp;		/* fifo stack pointer		 */
    usc		cm_ct;		/* command byte counter		 */
    usc		sis_force;	/* force sis command		 */
/* FDD INTERFACE */
    usc		cn[4];		/* cylinder number		 */    
    usc		pcn;
    usc		hds[4];		/* head number			 */
    void	(*fdd0)(void *);
    void	(*fdd1)(void *);
    void	(*fdd2)(void *);
    void	(*fdd3)(void *);
    void	(*irq)(void *);	/* Interrupt handler		 */
/* FDC PARAMETERS */
    usc		sc;		/* sectors/cylinder		 */
    usc		nb;		/* bytes/sector			 */
/* FDC COMMAND */
    usc		phase;		/* command phase		 */
    void	(*cmd)(void *); /* pointer to command function   */
/* FDD TRUE PARAMS */
    union{
	usc 	srt_hut;
	struct{
	    usc		hut:4;
	    usc		srt:4;
	};
    };
    union{
	usc	hlt_nd;
	struct{
	    usc		nd:1;
	    usc		hlt:7;
	};	
    };

} i8272;
/* command phase */
#define PH_COMMAND	0
#define PH_EXEC		1
#define PH_RESULT	2

/* MSR bitmask */
#define	MSR_RQM		0x80
#define MSR_DIO		0x40
#define MSR_ND		0x20
#define MSR_CB		0x10
#define MSR_DRV3_BUSY	0x08
#define MSR_DRV2_BUSY	0x04
#define MSR_DRV1_BUSY	0x02
#define MSR_DRV0_BUSY	0x01

/* ST0 bitmask */
#define ST0_IC_1	0x80
#define ST0_IC_0	0x40
#define ST0_SE		0x20
#define ST0_EC		0x10
#define ST0_NR		0x08
#define ST0_HD		0x04
#define ST0_US_1	0x02
#define ST0_US_0	0x01

/* ST1 bitmask */
#define ST1_EN		0x80 
//#define not used	0x40
#define ST1_DE		0x20
#define ST1_OR		0x10
//#define not used	0x08
#define ST1_ND		0x04
#define ST1_NW		0x02
#define ST1_MA		0x01

/* ST2 bitmask */
//#define not used always 0	0x80
#define ST2_CM		0x40
#define ST2_DD		0x20
#define ST2_WC		0x10
#define ST2_SH		0x08
#define ST2_SN		0x04
#define ST2_BC		0x02
#define ST2_MD		0x01

/* ST3 bitmask */
#define ST3_FT		0x80
#define ST3_WP		0x40
#define ST3_RDY		0x20
#define ST3_T0		0x10
#define ST3_TS		0x08
#define ST3_HD		0x04
#define ST3_US_1	0x02
#define ST3_US_0	0x01

/* i8272_init(fdd_0,fdd_1) */
#define i8272_ini(a,b)		_i8272_ini_(NULL,a,b,NULL,NULL,NULL)
#define i8272_rst()		_i8272_rst_(NULL)
#define i8272_tc()		_i8272_tc_(NULL)
#define i8272_ck()		_i8272_ck_(NULL)
#define i8272_rd(addr)		_i8272_rd_(NULL,addr)
#define i8272_wr(addr,val)	_i8272_wr_(NULL,addr,val)


/* i8272_ini(i8272, fdd0,fdd1,fdd2,fdd3, irq ) */
extern void _i8272_ini_(i8272*,void*,void*,void*,void*,void*);  /* init				*/
extern void _i8272_rst_(i8272*);				/* reset controller		*/
extern void _i8272_tc_(i8272*);				     	/* terminate command		*/
extern void _i8272_ck_(i8272*);				     	/* single controller step	*/
extern usc  _i8272_rd_(i8272*,usi);			     	/* read fifo/msr		*/
extern void _i8272_wr_(i8272*,usi,usc);			     	/* write to fifo		*/

#endif
