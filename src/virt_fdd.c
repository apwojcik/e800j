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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "e800j_config.h"
#include "virt_fdd.h"
#include "i8272_fdc.h"

static int vfdd_open_image(struct vfdd_disk* dsk, const char* path, unsigned char write_mode) {
    const unsigned char requested = write_mode;
    if ((dsk->f = fopen(path, write_mode != 0 ? "rb+" : "rb")) == nullptr) {
        if ((dsk->f = fopen(path, "rb")) == nullptr) {
            return -1;
        }
        write_mode = 0;
    }
    const size_t elements = fread(&dsk->hdr, sizeof(struct vfdd_hdr), 1, dsk->f);
    if (elements != 1) {
        memset(&dsk->hdr, 0, sizeof(struct vfdd_hdr));
    }
    dsk->stat = write_mode ? 0 : VFDD_WPROT;
    dsk->sbyte = 0;
    dsk->lbytes = 0;
    dsk->rw = requested;
    snprintf(dsk->path, sizeof(dsk->path), "%s", path);
    return FDD_OK;
}

static void vfdd_close_image(struct vfdd_disk* dsk) {
    fclose(dsk->f);
    dsk->stat = VFDD_DOPEN;
    dsk->f = nullptr;
}

static int vfdd_seek_track(struct vfdd_disk* dsk, const int trk, const int hd) {
    if (fseek(dsk->f, 0x0L, SEEK_END) != 0) {
        return -1;
    }
    const long fsize = ftell(dsk->f);
    /* ustawienie na ścieżkę 00 */
    if (fseek(dsk->f, 0x100, SEEK_SET) != 0) {
        return -1;
    }
    dsk->sbyte = 0;
    dsk->seek = 0x100;
    while (feof(dsk->f) == 0) {
        /* odczyt nagłówka ścieżki */
        if (fread(&dsk->th, sizeof(struct vfdd_trk_hdr), 1, dsk->f) != 1)
            return -1;
        if (dsk->th.tr_nb == trk && dsk->th.hd_nb == hd) {
            break;
        }
        dsk->seek = dsk->seek + (0x80 << dsk->th.bps) * dsk->th.spt + 0x100;
        if (dsk->seek >= fsize) {
            return -1;
        }
        if (fseek(dsk->f, dsk->seek, SEEK_SET)) {
            return -1;
        }
    }
    return feof(dsk->f);
}

static int vfdd_seek_sector(struct vfdd_disk* dsk, const int trk, const int sec, const int hd) {
    vfdd_seek_track(dsk, trk, hd);
    if (fseek(dsk->f, dsk->seek + sizeof(struct vfdd_trk_hdr), SEEK_SET)) {
        return -1;
    }
    /* przeszukuje tablicę w poszukiwaniu właściwego sektora */
    for (int i = 0; i < dsk->th.spt; ++i) {
        if (feof(dsk->f)) {
            return FDD_ERR_OVER;
        }
        if (fread(&dsk->sh, sizeof(struct vfdd_sec_hdr), 1, dsk->f) != 1) {
            return FDD_ERR_OVER;
        }
        if (dsk->sh.tr_nb == trk && dsk->sh.sc_nb == sec && dsk->sh.hd_nb == hd) {
            /* ustawia na początek danych sektora */
            dsk->lbytes = (0x80 << dsk->th.bps);
            if (fseek(dsk->f, dsk->seek + 0x100 + dsk->lbytes * i, SEEK_SET)) {
                return FDD_ERR_OVER;
            }
            dsk->sbyte = 0;
            return FDD_OK;
        }
    }
    return FDD_ERR_OVER; /* Nie znaleziono sektora */
}

static int vfdd_read_sector(struct vfdd_disk* dsk) {
    if ((dsk->stat & VFDD_DOPEN) != 0 || dsk->f == nullptr) {
        return FDD_ERR_NODISK;
    }
    const int data = fgetc(dsk->f);
    if (++dsk->sbyte > 0x80 << dsk->th.bps) {
        return FDD_ERR_OVER;
    }
    return data;
}

static int vfdd_write_sector(struct vfdd_disk* dsk, const int data) {
    if ((dsk->stat & VFDD_DOPEN) != 0 || dsk->f == nullptr) {
        return FDD_ERR_NODISK;
    }
    if ((dsk->stat & VFDD_WPROT) != 0) {
        return FDD_ERR_WPROT;
    }
    if (feof(dsk->f) != 0) {
        return FDD_ERR_OVER;
    }
    fputc(data, dsk->f);
    if (++dsk->sbyte > 0x80 << dsk->th.bps) {
        return FDD_ERR_OVER;
    }
    return FDD_OK;
}

static_assert(sizeof(struct vfdd_hdr) == 0x100, "nagłówek DSK musi mieć 0x100 bajtów");
static_assert(sizeof(struct vfdd_trk_hdr) == 0x18, "nagłówek ścieżki DSK musi mieć 0x18 bajtów");
static_assert(sizeof(struct vfdd_sec_hdr) == 8, "opis sektora DSK musi mieć 8 bajtów");

static int vfdd_truncate(FILE* f, const long len) {
    if (fflush(f) != 0) {
        return -1;
    }
#ifdef _WIN32
    return _chsize_s(_fileno(f), len) == 0 ? 0 : -1;
#else
    return ftruncate(fileno(f), (off_t)len) == 0 ? 0 : -1;
#endif
}

/* przenosi len bajtów z from na to (obszary mogą się nakładać), 0 = OK */
static int vfdd_move(FILE* f, const long from, const long to, const long len) {
    unsigned char buf[8192];
    if (len <= 0 || from == to) {
        return 0;
    }
    if (to < from) {
        for (long done = 0; done < len;) {
            const size_t n = (size_t)(len - done < (long)sizeof buf ? len - done : (long)sizeof buf);
            if (fseek(f, from + done, SEEK_SET) != 0 || fread(buf, 1, n, f) != n ||
                fseek(f, to + done, SEEK_SET) != 0 || fwrite(buf, 1, n, f) != n) {
                return -1;
            }
            done += (long)n;
        }
    } else {
        /* przesunięcie w górę: od końca, żeby nie nadpisać jeszcze niescopiowanych danych */
        for (long left = len; left > 0;) {
            const size_t n = (size_t)(left < (long)sizeof buf ? left : (long)sizeof buf);
            left -= (long)n;
            if (fseek(f, from + left, SEEK_SET) != 0 || fread(buf, 1, n, f) != n ||
                fseek(f, to + left, SEEK_SET) != 0 || fwrite(buf, 1, n, f) != n) {
                return -1;
            }
        }
    }
    return 0;
}

int vfdd_format_track(struct vfdd_disk* dsk, const struct vfdd_fmt* fmt) {
    FILE* f = dsk->f;
    if (f == nullptr || (dsk->stat & VFDD_DOPEN) != 0) {
        return FDD_ERR_NODISK;
    }
    if ((dsk->stat & VFDD_WPROT) != 0) {
        return FDD_ERR_WPROT;
    }
    if (fmt->n > VFDD_MAX_N || fmt->sc > VFDD_MAX_SPT) {
        return FDD_ERR_OVER;
    }

    /* nowy blok ścieżki: nagłówek 0x100 (z listą sektorów) + dane */
    const long secsize = 0x80L << fmt->n;
    const long newsize = 0x100 + secsize * fmt->sc;
    unsigned char* blk = malloc((size_t)newsize);
    if (blk == nullptr) {
        return FDD_ERR_OVER;
    }
    memset(blk, 0, 0x100);
    memcpy(blk, "Track-Info\r\n", 13);
    blk[0x10] = fmt->trk;
    blk[0x11] = fmt->hd;
    blk[0x14] = fmt->n;
    blk[0x15] = fmt->sc;
    blk[0x16] = fmt->gap3;
    blk[0x17] = fmt->filler;
    for (int i = 0; i < fmt->sc; ++i) {
        unsigned char* sh = blk + sizeof(struct vfdd_trk_hdr) + (size_t)i * sizeof(struct vfdd_sec_hdr);
        memcpy(sh, fmt->ids + i * 4, 4);
        sh[6] = (unsigned char)(secsize & 0xff); /* sec_len, little endian */
        sh[7] = (unsigned char)(secsize >> 8);
    }
    memset(blk + 0x100, fmt->filler, (size_t)(secsize * fmt->sc));

    int rc = FDD_ERR_OVER;
    if (fseek(f, 0, SEEK_END) != 0) {
        goto done;
    }
    long fsize = ftell(f);
    if (fsize < 0) {
        goto done;
    }
    if (fsize < 0x100) {
        /* pusty plik: nowy nagłówek obrazu */
        memset(&dsk->hdr, 0, sizeof dsk->hdr);
        memcpy(dsk->hdr.format_name, "MV - CPCEMU Disk-File\r\nDisk-Info\r\n", 34);
        snprintf(dsk->hdr.creator, sizeof dsk->hdr.creator, "e800j");
        if (fseek(f, 0, SEEK_SET) != 0 || fwrite(&dsk->hdr, sizeof dsk->hdr, 1, f) != 1) {
            goto done;
        }
        fsize = 0x100;
    }

    /* szukanie miejsca: istniejąca ścieżka (zastąpienie) albo pierwsza o wyższym numerze (wstawienie) */
    long pos = 0x100;
    long off = -1;
    long old = 0;
    struct vfdd_trk_hdr th;
    while (pos + (long)sizeof th <= fsize) {
        if (fseek(f, pos, SEEK_SET) != 0 || fread(&th, sizeof th, 1, f) != 1) {
            break;
        }
        if (memcmp(th.info, "Track-Info", 10) != 0 || th.bps > VFDD_MAX_N) {
            break;
        }
        if (th.tr_nb == fmt->trk && th.hd_nb == fmt->hd) {
            off = pos;
            old = 0x100 + (0x80L << th.bps) * th.spt;
            break;
        }
        if (th.tr_nb > fmt->trk || (th.tr_nb == fmt->trk && th.hd_nb > fmt->hd)) {
            off = pos;
            break;
        }
        pos += 0x100 + (0x80L << th.bps) * th.spt;
    }
    if (off < 0) {
        off = pos < fsize ? pos : fsize; /* na końcu obrazu */
    }
    long tail = fsize - (off + old);
    if (tail < 0) {
        tail = 0;
    }

    if (vfdd_move(f, off + old, off + newsize, tail) != 0) {
        goto done;
    }
    if (fseek(f, off, SEEK_SET) != 0 || fwrite(blk, 1, (size_t)newsize, f) != (size_t)newsize) {
        goto done;
    }
    if (off + newsize + tail < fsize && vfdd_truncate(f, off + newsize + tail) != 0) {
        goto done;
    }

    /* nagłówek obrazu: liczba ścieżek/głowic i maksymalny rozmiar ścieżki */
    if (fmt->trk >= dsk->hdr.tps) {
        dsk->hdr.tps = (unsigned char)(fmt->trk + 1);
    }
    if (fmt->hd >= dsk->hdr.hpd) {
        dsk->hdr.hpd = (unsigned char)(fmt->hd + 1);
    }
    if (newsize > (dsk->hdr.size_1 | (dsk->hdr.size_2 << 8))) {
        dsk->hdr.size_1 = (unsigned char)(newsize & 0xff);
        dsk->hdr.size_2 = (unsigned char)(newsize >> 8);
    }
    if (fseek(f, 0, SEEK_SET) != 0 || fwrite(&dsk->hdr, sizeof dsk->hdr, 1, f) != 1 || fflush(f) != 0) {
        goto done;
    }
    dsk->sbyte = 0;
    rc = FDD_OK;
done:
    free(blk);
    return rc;
}

/***********************************/

int vfdd_dsk_floppy(struct vfdd_disk* dsk, const char action,
    const unsigned char parm0, const unsigned char parm1, const unsigned char parm2, const char* path)
{
    switch (action) {
    case VFDD_INIT:
        dsk->f = nullptr;
        dsk->sbyte = 0;
        dsk->lbytes = 0;
        dsk->stat = 0;
        dsk->path[0] = 0;
        dsk->rw = VFDD_RO;
        break;
    case VFDD_LOAD:
        if (dsk->f == nullptr) {
            return vfdd_open_image(dsk, path, parm0);
        }
        break;
    case VFDD_UNLOAD:
        if (dsk->f != nullptr) {
            vfdd_close_image(dsk);
        }
        break;
    case VFDD_SEEK_TRK:
        if (dsk->f != nullptr) {
            return vfdd_seek_track(dsk, parm0, parm2);
        }
        break;
    case VFDD_SEEK_SEC:
        if (dsk->f != nullptr) {
            return vfdd_seek_sector(dsk, parm0, parm1, parm2);
        }
        break;
    case VFDD_RD_SEC:
        if (dsk->f != nullptr) {
            return vfdd_read_sector(dsk);
        }
        return 0x20;
    case VFDD_WR_SEC:
        if (dsk->f != nullptr) {
            return vfdd_write_sector(dsk, parm0);
        }
        break;
    default:
        break;
    }
    return 0;
}

int vfdd_swap(struct vfdd_disk* dsk, const char* path, const unsigned char write_mode) {
    if (dsk->f != nullptr) {
        /* ten sam plik może zostać otwarty ponownie (zmiana RO/RW), więc zapisz bufor */
        fflush(dsk->f);
    }
    struct vfdd_disk next = *dsk;
    next.f = nullptr;
    if (vfdd_open_image(&next, path, write_mode) < 0) {
        return -1;
    }
    if (dsk->f != nullptr) {
        fclose(dsk->f);
    }
    *dsk = next;
    return FDD_OK;
}

int vfdd_drive(const void* ptr, void* dsk, const unsigned char action, const unsigned char wdata) {
    const struct i8272* fdc = ptr;
    switch (action) {
    case FDD_SEEK:
        return vfdd_dsk_floppy(dsk, VFDD_SEEK_TRK, fdc->cn, 0, fdc->ha, nullptr);
    case FDD_SEEK_SECTOR:
        return vfdd_dsk_floppy(dsk, VFDD_SEEK_SEC, fdc->cn, fdc->rec, fdc->ha, nullptr);
    case FDD_READ_TRACK:
        return vfdd_dsk_floppy(dsk, VFDD_RD_SEC, 0, 0, 0, nullptr);
    case FDD_WRITE_TRACK:
        return vfdd_dsk_floppy(dsk, VFDD_WR_SEC, wdata, 0, 0, nullptr);
    case FDD_FORMAT_TRACK: {
        const struct vfdd_fmt fmt = {
            .trk = fdc->pcn, .hd = fdc->hds, .n = fdc->nb, .sc = fdc->eot,
            .gap3 = fdc->gpl, .filler = fdc->dtl, .ids = fdc->fmt_id,
        };
        return vfdd_format_track(dsk, &fmt);
    }
    default:
        return FDD_OK;
    }
}
