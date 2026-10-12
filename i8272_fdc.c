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

#define DEBUG_MODE

#ifdef DEBUG_MODE
#include "e800j_msg.h"
#endif

#include "i8272_fdc.h"

/* tablica dekodująca instrukcje */
static const i8272opc opcodes_tabl[32] = i8272_CMD_TABLE;

void i8272_command(i8272 *fdc,  char plc);
void i8272_result(i8272 *fdc, char plc);
void i8272_write_data(i8272 *fdc, char mode);
int i8272_dummy_drive(void *f, void *param, uchar action, uchar wdata) { return 0x20; }

/**************************************************************************************************/

void i8272_fdd_connect
(i8272 *fdc, int (*drive)(void*,void*,uchar,uchar), void *f0, void *f1, void *f2,void *f3)
{
    fdc->drive=drive;
    fdc->parameter[0] = f0;
    fdc->parameter[1] = f1;
    fdc->parameter[2] = f2;
    fdc->parameter[3] = f3;
}

void i8272_init(i8272 *fdc)
{
    int i;

    fdc->msr = 0x80;
    fdc->st0 = 0x80;
    fdc->st1 = 0x00;
    fdc->st2 = 0x00;
    fdc->st3 = 0x00;

    fdc->srt = 0x00;
    fdc->hut = 0x00;
    fdc->hlt = 0x00;
    fdc->hds = 0x00;
    fdc->pcn = 0x00;
    fdc->ds  = 0x00;

    fdc->icntr = 0x00;
    fdc->phase = FDC_COMMAND;
    fdc->data  = 0x00;
    for(i=0; i< 16; i++) fdc->buffer[i] = 0x00;
    fdc->idx_0 = fdc->idx_1 = 0;
    fdc->opcode = (void*)0;
    fdc->parameter[0] = (void*)0;
    fdc->parameter[1] = (void*)0;
    fdc->parameter[2] = (void*)0;
    fdc->parameter[3] = (void*)0;
    fdc->drive = i8272_dummy_drive;
    fdc->int_line=0;
    fdc->rw = 0;
    msg_printf(MSG_MSG,"FDC i8272A init\n");
}

/* reakcja na stan linii RESET */
void i8272_reset(i8272 *fdc, uchar reset)
{
    int i;

    if(!reset) return;

    fdc->msr = (fdc->msr & FDC_NDM) | 0x80;
    fdc->st0 = 0x80;
    fdc->st1 = 0x00;
    fdc->st2 = 0x00;
    fdc->st3 = 0x00;
    fdc->err = 0;

    fdc->hds = 0x00;
    fdc->pcn = 0x00;
    fdc->ds  = 0x00;

    fdc->icntr = 0x00;
    fdc->phase = FDC_COMMAND;
    fdc->data  = 0x00;
    for(i=0; i< 16; i++) fdc->buffer[i] = 0x00;
    fdc->idx_0 = fdc->idx_1 = 0;
    fdc->opcode = (void*)0;
    fdc->int_line=0;
    fdc->rw = 0;
    msg_printf(MSG_MSG,"FDC i8272A reset\n");
}

/* reakcja na stan linii TC, zakończenie bieżącej komendy jeśli tc=1 */
void i8272_tc(i8272 *fdc, uchar tc)
{
    if(!tc) return;
    if(!fdc->rw){
        fdc->phase  = FDC_COMMAND;
        fdc->opcode = (void*)0;
        fdc->icntr  = 0;
        fdc->msr &= ~(FDC_DIO | FDC_RQM);
        fdc->msr |= FDC_RQM;
        fdc->int_line=0;
    } else {
        fdc->rw = 2;
        fdc->msr |= FDC_RQM | FDC_DIO;
    }
    msg_printf(MSG_NVERB_MSG,"FDC i8272A TC received\n");
}

void i8272_call(i8272 *fdc, char plc)
{
    fdc->call = plc;
    if(fdc->opcode) fdc->opcode->command(fdc);
}

/* odczyt rejestru */
uchar i8272_in(i8272 *fdc, uchar a0)
{
    char tmp = 0xff;

    fdc->int_line=0;
    if(!(a0 & 1)){
        msg_printf(MSG_UNVERB_MSG,"FDC IN MSR %x\n", (unsigned char)fdc->msr);
        return fdc->msr;
    }

    if((fdc->msr & (FDC_RQM | FDC_DIO)) != (FDC_RQM | FDC_DIO)) return 0xff;

    if(fdc->rw)  i8272_result(fdc, FDC_IN_DATA_1);
    tmp = fdc->data;
    if(!fdc->rw)
    {
        fdc->msr &= ~FDC_RQM;
        i8272_result(fdc, FDC_IN_DATA_1);
    }
    msg_printf(MSG_UNVERB_MSG,"FDC IN DATA %x\n", (unsigned char)tmp);
    return tmp;
}

/* zapis bajtu do rejestru */
void i8272_out(i8272 *fdc, unsigned char address_bit, unsigned char data)
{
    fdc->int_line=0;
    if(!(address_bit & 1)){
        msg_printf(MSG_ERROR, "FDC Illegal port operation! \n");
        return;
    }
    if((fdc->msr & FDC_RQM) != FDC_RQM)  return;
    if(!fdc->rw) i8272_command(fdc, FDC_OUT_DATA_0);
    fdc->data = data;
    fdc->msr &= ~FDC_RQM;
    if(!fdc->rw)
        i8272_command(fdc, FDC_OUT_DATA_1);
    else
        i8272_call(fdc, FDC_OUT_DATA_1);
    msg_printf(MSG_UNVERB_MSG,"FDC OUT PORT %x\n", data);
}

/* wystawienie przerwania */
void i8272_int(i8272 *fdc, uchar icode)
{
    fdc->int_line = 1;
    fdc->msr |= FDC_RQM;
    fdc->st0 &= ~FDC_IC;
    fdc->st0 |= (icode & 0x03) << 6;
}

/* zakończenie komendy */
void i8272_end_cmd(i8272 *fdc)
{
    msg_printf(MSG_NVERB_MSG, "FDC end command\n");
    fdc->opcode = 0;
    fdc->phase = FDC_COMMAND;
    fdc->msr |= FDC_RQM;
    fdc->msr &= ~FDC_DIO;
    fdc->msr &= ~FDC_CB;
}

void i8272_command(i8272 *fdc,  char plc)
{
    if(fdc->phase != FDC_COMMAND) return;
    if(plc == FDC_OUT_DATA_1){
    if(!fdc->opcode){	/* pobranie pierwszego bajtu rozkazu */
        fdc->opcode = (i8272opc *)&opcodes_tabl[fdc->data & 0x1f];
        fdc->idx_0 = 0;
        fdc->rw = 0;
        fdc->err = 0;
        fdc->msr |= FDC_CB; /* ? */
        fdc->idx_1 = fdc->opcode->cmd_len;
    }

    fdc->buffer[fdc->idx_0++] = fdc->data;
    if(fdc->idx_0 == fdc->idx_1){
        fdc->phase = FDC_EXEC;
        fdc->msr &= ~FDC_RQM;
        fdc->icntr = 0;
        fdc->rw = 0;
        msg_printf(MSG_VERB_MSG,"FDC execute command %s\n",fdc->opcode->name);
        fdc->opcode->command(fdc);
        return;
    }
    fdc->msr |= FDC_RQM;
    fdc->msr &= ~FDC_DIO;
    }
}

/* wystawienie danych w fazie result */
void i8272_result(i8272 *fdc, char plc)
{
    if(fdc->rw && fdc->opcode && fdc->phase == FDC_EXEC){
        fdc->opcode->command(fdc);
        return;
    }
    if(plc != FDC_CMD && plc != FDC_IN_DATA_1) return;
    if(fdc->idx_0 == fdc->idx_1){
        i8272_end_cmd(fdc);
        return;
    }
    fdc->data = fdc->buffer[(fdc->idx_0++) & 0x0f];
    fdc->msr |= FDC_RQM | FDC_DIO;
}

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/

void i8272_rdata(i8272 *fdc, char mode)
{
    int tmp=0;

    fdc->st0 = 0;
    fdc->st1 = 0;
    fdc->st2 = 0;
    fdc->st3 = FDC_RDY;
/* odczyt bajtu z FDD */
    if(!fdc->err)
        tmp = fdc->drive(fdc, fdc->parameter[fdc->ds], FDD_READ_TRACK, 0x00);
    fdc->data = tmp;
    if(tmp < 0) fdc->err = tmp;
/* reakcja na brak dysku */
    if(fdc->err == FDD_ERR_NODISK){
        fdc->st0 = FDC_NR | (fdc->ds & 0x03) | 0x40;
        fdc->st1 = 0;
        fdc->st2 = 0;
        fdc->st3 = 0;
    }
/* ustawienie flag gotowości danej do odbioru przez CPU */
    fdc->msr |= FDC_RQM;
    fdc->msr |= FDC_DIO;
/* kolejny bajt do odczytu w sektorze */
    fdc->sbyte++;
/* jeśli odczyt kolejnego sektora zeruj licznik bajtów i zwiększ nr sektora o 1 */
    if(fdc->sbyte >= fdc->slen ){
        fdc->sbyte = 0;
        fdc->rec++;
    }
/*
  Przypuszczam, że po EOT zerowany jest licznik sektorów, ale słowa brak o tym w dokumentacji i8272.
*/
    if(fdc->rec > fdc->eot) fdc->rec = 1;
}

void i8272_read_data(i8272 *fdc, char mode)
{
    if(fdc->rw == 1){
        i8272_rdata(fdc, mode);
        return;
    }

    if(fdc->rw == 2){
        fdc->idx_0 = 0;
        fdc->idx_1 = fdc->opcode->res_len;
        fdc->opcode = (void *)0;
        fdc->buffer[0] = fdc->st0;
        fdc->buffer[1] = fdc->st1;
        fdc->buffer[2] = fdc->st2;
        fdc->buffer[3] = fdc->cn;
        fdc->buffer[4] = fdc->ha;
        fdc->buffer[5] = fdc->rec;
        fdc->buffer[6] = fdc->nb;
        fdc->phase = FDC_RESULT;
        fdc->rw = 0;
        i8272_result(fdc, FDC_CMD);
        i8272_int(fdc, FDC_IC_IC);
        return;
    }

    if(fdc->phase != FDC_EXEC) return;
    fdc->ds = fdc->buffer[1] & 0x03;
    fdc->hds = (fdc->buffer[1] >> 2) & 0x01;
    fdc->cn  = fdc->buffer[2];
    fdc->ha  = fdc->buffer[3];
    fdc->rec = fdc->buffer[4];
    fdc->nb  = fdc->buffer[5];
    fdc->eot = fdc->buffer[6];
    fdc->gpl = fdc->buffer[7];
    fdc->dtl = fdc->buffer[8];
    fdc->rw  = 1;
/* wyzerowanie licznika bajtów w sektorze */
    fdc->sbyte = 0;
/* obliczenie długości sektora w bajtach */
    fdc->slen = 0x80 << fdc->nb;
    if(!fdc->nb) fdc->slen = fdc->dtl;
/* obliczenie długości ścieżki */
    fdc->trlen = fdc->eot * fdc->slen;
/* ustawienie flag odbioru danej, dana będzie wstawiona dopiero po wywołaniu in(),
   ale inaczej by do niej nie weszła
 */
    fdc->msr |= FDC_RQM | FDC_DIO;
    fdc->drive(fdc, fdc->parameter[fdc->ds], FDD_SEEK_SECTOR, 0x00);

    msg_printf(MSG_NVERB_MSG,"D:%i C:%i H%i R:%i N:%i EOT:%i GPL:%i DTL:%i\n",
        fdc->ds, fdc->cn,fdc->ha, fdc->rec,fdc->nb,fdc->eot,fdc->gpl,fdc->dtl );
}


void i8272_rd_cmd(i8272* fdc){  i8272_read_data(fdc, 0);  }
void i8272_rdd_cmd(i8272* fdc){ i8272_read_data(fdc, 1);  }
void i8272_wd_cmd(i8272* fdc){  i8272_write_data(fdc, 0); }
void i8272_wdd_cmd(i8272* fdc){ i8272_write_data(fdc, 1); }
void i8272_rt_cmd(i8272* fdc){  i8272_read_data(fdc, 2);  }


void i8272_sis_cmd(i8272* fdc)
{
    if(fdc->phase == FDC_COMMAND) return;

    fdc->idx_0 = 0;
    fdc->idx_1 = fdc->opcode->res_len;
    fdc->opcode = (void *)0;
    fdc->buffer[0] = fdc->st0;
    fdc->buffer[1] = fdc->pcn;
    fdc->phase = FDC_RESULT;
    fdc->st0 &= ~FDC_SE;
    i8272_result(fdc, FDC_CMD);
    i8272_int(fdc, FDC_IC_IC);
}

void i8272_specify_cmd(i8272* fdc)
{
    if(fdc->phase == FDC_COMMAND) return;
    fdc->srt = fdc->buffer[1];
    fdc->hut = fdc->srt & 0x0f;
    fdc->srt >>= 4;
    fdc->hlt = fdc->buffer[2];
    fdc->msr &= ~FDC_NDM;
    fdc->msr |= (fdc->hlt & 1) ? FDC_NDM : 0;
    fdc->hlt >>= 1;

    i8272_end_cmd(fdc);
    i8272_int(fdc, FDC_IC_NT);
}

void i8272_seek_cmd(i8272* fdc)
{
    if(fdc->phase == FDC_COMMAND) return;
    fdc->ds  = fdc->buffer[1] & 0x03;
    fdc->hds = (fdc->buffer[1] >> 2) & 0x01;
    fdc->pcn = fdc->buffer[2];
    fdc->drive(fdc, fdc->parameter[fdc->ds], FDD_SEEK, 0x00);

    i8272_end_cmd(fdc);
    i8272_int(fdc, FDC_IC_NT);
}

void i8272_rec_cmd(i8272* fdc)
{
    if(fdc->phase == FDC_COMMAND) return;
    fdc->ds  = fdc->buffer[1] & 0x03;
    fdc->pcn = 0;
    fdc->drive(fdc, fdc->parameter[fdc->ds], FDD_SEEK, 0x00);

    i8272_end_cmd(fdc);
    i8272_int(fdc, FDC_IC_NT);
}

void i8272_sds_cmd(i8272* fdc)
{
    if(fdc->phase == FDC_COMMAND) return;
    fdc->ds  = fdc->buffer[1] & 0x03;
    fdc->hds = (fdc->buffer[1] >> 2) & 0x01;
    fdc->idx_0 = 0;
    fdc->idx_1 = fdc->opcode->res_len;
    fdc->opcode = (void *)0;
    fdc->buffer[0] = fdc->st3;
    fdc->phase = FDC_RESULT;
    i8272_result(fdc, FDC_CMD);
    i8272_int(fdc, FDC_IC_NT);
}

void i8272_invalid_cmd(i8272* fdc)
{
    if(fdc->phase == FDC_COMMAND) return;
    fdc->idx_0 = 0;
    fdc->idx_1 = fdc->opcode->res_len;
    fdc->opcode = (void *)0;
    fdc->buffer[0] = fdc->st0;
    fdc->phase = FDC_RESULT;
    i8272_result(fdc, FDC_CMD);
    fdc->msr |= FDC_RQM | FDC_DIO;
    fdc->st0 = 0x80;
}

void i8272_wdata(i8272 *fdc, char mode)
{
/* odczyt bajtu z FDD */
    fdc->st0 = 0;
    fdc->st1 = 0;
    fdc->st2 = 0;
    fdc->st3 = FDC_RDY;
    if(!fdc->err)
        fdc->err = fdc->drive(fdc, fdc->parameter[fdc->ds], FDD_WRITE_TRACK, fdc->data);
/* reakcja na blokadę zapisu */
    if(fdc->err == FDD_ERR_WPROT){
        fdc->st0 = FDC_NR | (fdc->ds & 0x03) | 0x40;
        fdc->st1 = FDC_NW;
        fdc->st2 = 0;
        fdc->st3 = FDC_WP;
    }
/* reakcja na brak dysku */
    if(fdc->err == FDD_ERR_NODISK){
        fdc->st0 = FDC_NR | (fdc->ds & 0x03) | 0x40;
        fdc->st1 = 0;
        fdc->st2 = 0;
        fdc->st3 = 0;
    }

/* ustawienie flag gotowości danej do zapisu przez CPU */
    fdc->msr |= FDC_RQM;
    fdc->msr &= ~FDC_DIO;
/* kolejny bajt do odczytu w sektorze */
    fdc->sbyte++;
/* jeśli odczyt kolejnego sektora zeruj licznik bajtów i zwiększ nr sektora o 1 */
    if(fdc->sbyte >= fdc->slen ){
        fdc->sbyte = 0;
        fdc->rec++;
    }

    if(fdc->rec > fdc->eot) fdc->rec = 1;
}

void i8272_write_data(i8272 *fdc, char mode)
{
    if(fdc->rw == 1){
        i8272_wdata(fdc, mode);
        return;
    }

    if(fdc->rw == 2){
        fdc->idx_0 = 0;
        fdc->idx_1 = fdc->opcode->res_len;
        fdc->opcode = (void *)0;
        fdc->buffer[0] = fdc->st0;
        fdc->buffer[1] = fdc->st1;
        fdc->buffer[2] = fdc->st2;
        fdc->buffer[3] = fdc->cn;
        fdc->buffer[4] = fdc->ha;
        fdc->buffer[5] = fdc->rec;
        fdc->buffer[6] = fdc->nb;
        fdc->phase = FDC_RESULT;
        fdc->rw = 0;
        i8272_result(fdc, FDC_CMD);
        i8272_int(fdc, FDC_IC_IC);
        return;
    }

    if(fdc->phase != FDC_EXEC) return;
    fdc->ds = fdc->buffer[1] & 0x03;
    fdc->hds = (fdc->buffer[1] >> 2) & 0x01;
    fdc->cn  = fdc->buffer[2];
    fdc->ha  = fdc->buffer[3];
    fdc->rec = fdc->buffer[4];
    fdc->nb  = fdc->buffer[5];
    fdc->eot = fdc->buffer[6];
    fdc->gpl = fdc->buffer[7];
    fdc->dtl = fdc->buffer[8];
    fdc->rw  = 1;
/* wyzerowanie licznika bajtów w sektorze */
    fdc->sbyte = 0;
/* obliczenie długości sektora w bajtach */
    fdc->slen = 0x80 << fdc->nb;
    if(!fdc->nb) fdc->slen = fdc->dtl;
/* obliczenie długości ścieżki */
    fdc->trlen = fdc->eot * fdc->slen;
/* ustawienie flag możliwości zapisu danej  */
    fdc->msr |= FDC_RQM;
    fdc->msr &= ~FDC_DIO;
    fdc->drive(fdc, fdc->parameter[fdc->ds], FDD_SEEK_SECTOR, 0x00);

    msg_printf(MSG_NVERB_MSG,"D:%i C:%i H%i R:%i N:%i EOT:%i GPL:%i DTL:%i\n",
        fdc->ds, fdc->cn,fdc->ha, fdc->rec,fdc->nb,fdc->eot,fdc->gpl,fdc->dtl);
}

/************************************************************************************************/

/* nie wspierane */
void i8272_rid_cmd(i8272* fdc)
{
    msg_printf(MSG_ERROR,"Unsupported command\n");
    i8272_invalid_cmd(fdc);
}

void i8272_ft_cmd(i8272* fdc)
{
    msg_printf(MSG_ERROR,"Unsupported command\n");
    i8272_invalid_cmd(fdc);
}

void i8272_seq_cmd(i8272* fdc)
{
    msg_printf(MSG_ERROR,"Unsupported command\n");
    i8272_invalid_cmd(fdc);
}

void i8272_sleq_cmd(i8272* fdc)
{
    msg_printf(MSG_ERROR,"Unsupported command\n");
    i8272_invalid_cmd(fdc);
}

void i8272_sheq_cmd(i8272* fdc)
{
    msg_printf(MSG_ERROR,"Unsupported command\n");
    i8272_invalid_cmd(fdc);
}

