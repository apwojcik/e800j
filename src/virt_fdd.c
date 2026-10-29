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

#include <stdio.h>
#include <string.h>
#include "virt_fdd.h"
#include "i8272_fdc.h"

static unsigned char vfdd_open_image(struct vfdd_disk* dsk, const char* path, unsigned char write_mode) {
    if ((dsk->f = fopen(path, write_mode != 0 ? "r+" : "r")) == NULL) {
        if ((dsk->f = fopen(path, "r")) == NULL) {
            return 1;
        }
        write_mode = 0;
    }
    if (fread(&dsk->hdr, sizeof(struct vfdd_hdr), 1, dsk->f) != 1)
        memset(&dsk->hdr, 0, sizeof(struct vfdd_hdr));
    dsk->stat = write_mode ? 0 : VFDD_WPROT;
    dsk->sbyte = 0;
    dsk->lbytes = 0;
    return FDD_OK;
}

static void vfdd_close_image(struct vfdd_disk* dsk) {
    fclose(dsk->f);
    dsk->f = NULL;
    dsk->stat = VFDD_DOPEN;
}

static unsigned char vfdd_seek_track(struct vfdd_disk* dsk, const int trk, const int hd) {
    if (fseek(dsk->f, 0x0L, SEEK_END) != 0) {
        return 1;
    }
    const long fsize = ftell(dsk->f);
    /* ustawienie na ścieżkę 00 */
    if (fseek(dsk->f, 0x100, SEEK_SET) != 0) {
        return 1;
    }
    dsk->sbyte = 0;
    dsk->seek = 0x100;
    while (feof(dsk->f) == 0) {
        /* odczyt nagłówka ścieżki */
        if (fread(&dsk->th, sizeof(struct vfdd_trk_hdr), 1, dsk->f) != 1)
            return 1;
        if (dsk->th.tr_nb == trk && dsk->th.hd_nb == hd) {
            break;
        }
        dsk->seek = dsk->seek + (0x80 << dsk->th.bps) * dsk->th.spt + 0x100;
        if (dsk->seek >= fsize) {
            return 1;
        }
        if (fseek(dsk->f, dsk->seek, SEEK_SET)) {
            return 1;
        }
    }
    return (unsigned char)feof(dsk->f);
}

static unsigned char vfdd_seek_sector(struct vfdd_disk* dsk, const int trk, const int sec, const int hd) {
    vfdd_seek_track(dsk, trk, hd);
    if (fseek(dsk->f, dsk->seek + sizeof(struct vfdd_trk_hdr), SEEK_SET)) {
        return 1;
    }
    /* przeszukuje tablicę w poszukiwaniu właściwego sektora */
    for (int i = 0; i < dsk->th.spt; i++) {
        if (feof(dsk->f))
            return FDD_ERR_OVER;
        if (fread(&dsk->sh, sizeof(struct vfdd_sec_hdr), 1, dsk->f) != 1)
            return FDD_ERR_OVER;
        if (dsk->sh.tr_nb == trk && dsk->sh.sc_nb == sec && dsk->sh.hd_nb == hd) {
            /* ustawia na początek danych sektora */
            dsk->lbytes = (0x80 << dsk->th.bps);
            if (fseek(dsk->f, dsk->seek + 0x100 + dsk->lbytes * i, SEEK_SET))
                return FDD_ERR_OVER;
            dsk->sbyte = 0;
            return FDD_OK;
        }
    }
    return FDD_ERR_OVER; /* Nie znaleziono sektora */
}

static unsigned char vfdd_read_sector(struct vfdd_disk* dsk) {
    if (dsk->stat & VFDD_DOPEN || !dsk->f) {
        return FDD_ERR_NODISK;
    }
    const unsigned char data = (unsigned char)fgetc(dsk->f);
    if (++dsk->sbyte > (0x80 << dsk->th.bps)) {
        return FDD_ERR_OVER;
    }
    return data;
}

static unsigned char vfdd_write_sector(struct vfdd_disk* dsk, const unsigned char data) {
    if (dsk->stat & VFDD_DOPEN || !dsk->f)
        return FDD_ERR_NODISK;
    if (dsk->stat & VFDD_WPROT)
        return FDD_ERR_WPROT;
    if (feof(dsk->f))
        return FDD_ERR_OVER;
    fputc(data, dsk->f);
    if (++dsk->sbyte > (0x80 << dsk->th.bps))
        return FDD_ERR_OVER;
    return FDD_OK;
}

/***********************************/

unsigned char vfdd_dsk_floppy(struct vfdd_disk* dsk, const char action,
    const unsigned char parm0, const unsigned char parm1, const unsigned char parm2, const char* path)
{
    unsigned char ret = 0;
    switch (action) {
    case VFDD_INIT:
        dsk->f = NULL;
        dsk->sbyte = 0;
        dsk->lbytes = 0;
        dsk->stat = 0;
        break;
    case VFDD_LOAD:
        if (dsk->f == NULL) {
            ret = vfdd_open_image(dsk, path, parm0);
        }
        break;
    case VFDD_UNLOAD:
        if (dsk->f != NULL) {
            vfdd_close_image(dsk);
        }
        break;
    case VFDD_SEEK_TRK:
        if (dsk->f != NULL) {
            ret = vfdd_seek_track(dsk, parm0, parm2);
        }
        break;
    case VFDD_SEEK_SEC:
        if (dsk->f != NULL) {
            ret = vfdd_seek_sector(dsk, parm0, parm1, parm2);
        }
        break;
    case VFDD_RD_SEC:
        if (dsk->f != NULL) {
            ret = vfdd_read_sector(dsk);
        } else {
            ret = 0x20;
        }
        break;
    case VFDD_WR_SEC:
        if (dsk->f != NULL) {
            ret = vfdd_write_sector(dsk, parm0);
        }
        break;
    default:
        break;
    }
    return ret;
}

unsigned char vfdd_drive(const void* ptr, void* dsk, const unsigned char action, const unsigned char wdata) {
    const struct i8272* fdc = ptr;
    switch (action) {
    case FDD_SEEK:
        return vfdd_floppy(dsk, VFDD_SEEK_TRK, fdc->cn, 0, fdc->ha, 0);
    case FDD_SEEK_SECTOR:
        return vfdd_floppy(dsk, VFDD_SEEK_SEC, fdc->cn, fdc->rec, fdc->ha, 0);
    case FDD_READ_TRACK:
        return vfdd_floppy(dsk, VFDD_RD_SEC, 0, 0, 0, 0);
    case FDD_WRITE_TRACK:
        return vfdd_floppy(dsk, VFDD_WR_SEC, wdata, 0, 0, 0);
    default:
        return FDD_OK;
    }
}
