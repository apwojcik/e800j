/* Elwro 800 Junior emulator v0.3
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "e800j.h"
#include "e800j_disk.h"
#include "e800j_msg.h"
#include "virt_fdd.h"
#include "zx_scrn.h"

/* SDL_EVENT_USER jest zajęte przez zegar CPU/ekranu (zx_scrn.c), a SDL_RegisterEvents() zwraca właśnie je */
#define DISK_EVENT_TYPE (SDL_EVENT_USER + 1)

static const SDL_DialogFileFilter disk_filters[] = {
    { "CPCEmu disk image", "dsk" },
    { "All files", "*" },
};

static char drive_name(const int drive) {
    return (char)('A' + drive);
}

static const char* disk_basename(const char* path) {
    const char* base = path;
    for (const char* p = path; *p != 0; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

static void disk_insert(struct elwro* e, const int drive, const char* path) {
    struct vfdd_disk* disk = e->disk[drive];
    if (vfdd_swap(disk, path, disk->rw) < 0) {
        msg_printf(MSG_ERROR, "Drive %c: opening file %s failed.\n", drive_name(drive), path);
        screen_toast(e->scn, "Floppy insert failed: %c: %s", drive_name(drive), disk_basename(path));
        return;
    }
    msg_printf(MSG_MSG, "Drive %c: loaded %s (%s)\n", drive_name(drive), path,
        (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
    screen_toast(e->scn, "Floppy inserted: %c: %s (%s)", drive_name(drive), disk_basename(path),
        (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
}

static void disk_eject(struct elwro* e, const int drive) {
    struct vfdd_disk* disk = e->disk[drive];
    if (disk->f == nullptr) {
        msg_printf(MSG_MSG, "Drive %c: already empty\n", drive_name(drive));
        screen_toast(e->scn, "Floppy drive %c: already empty", drive_name(drive));
        return;
    }
    vfdd_dsk_floppy(disk, VFDD_UNLOAD, 0, 0, 0, nullptr);
    msg_printf(MSG_MSG, "Drive %c: ejected\n", drive_name(drive));
    screen_toast(e->scn, "Floppy ejected: %c: %s", drive_name(drive), disk_basename(disk->path));
}

static void disk_toggle_rw(struct elwro* e, const int drive) {
    struct vfdd_disk* disk = e->disk[drive];
    const unsigned char mode = disk->rw == VFDD_RW ? VFDD_RO : VFDD_RW;
    if (disk->f == nullptr) {
        disk->rw = mode;
        msg_printf(MSG_MSG, "Drive %c: next disk will be opened %s\n", drive_name(drive),
            mode == VFDD_RW ? "RW" : "RO");
        screen_toast(e->scn, "Floppy drive %c: next disk will be %s", drive_name(drive), mode == VFDD_RW ? "RW" : "RO");
        return;
    }
    if (vfdd_swap(disk, disk->path, mode) < 0) {
        msg_printf(MSG_ERROR, "Drive %c: reopening file %s failed.\n", drive_name(drive), disk->path);
        screen_toast(e->scn, "Floppy reopen failed: %c: %s", drive_name(drive), disk_basename(disk->path));
        return;
    }
    msg_printf(MSG_MSG, "Drive %c: %s reopened (%s)\n", drive_name(drive), disk->path,
        (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
    screen_toast(e->scn, "Floppy mode changed: %c: %s (%s)", drive_name(drive), disk_basename(disk->path),
        (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
}

static int disk_name_cmp(const void* a, const void* b) {
    return strcmp(*(const char* const*)a, *(const char* const*)b);
}

/* wkłada kolejny (w porządku alfabetycznym) plik *.dsk z katalogu diskdir */
static void disk_cycle(struct elwro* e, const int drive) {
    int count = 0;
    char** list = SDL_GlobDirectory(e->diskdir, "*.dsk", SDL_GLOB_CASEINSENSITIVE, &count);
    if (list == nullptr || count == 0) {
        msg_printf(MSG_ERROR, "No *.dsk files in %s\n", e->diskdir);
        screen_toast(e->scn, "No *.dsk files in %s", e->diskdir);
        SDL_free(list);
        return;
    }
    qsort(list, count, sizeof(*list), disk_name_cmp);
    const char* current = disk_basename(e->disk[drive]->path);
    int next = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(disk_basename(list[i]), current) == 0) {
            next = (i + 1) % count;
            break;
        }
    }
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", e->diskdir, list[next]);
    SDL_free(list);
    disk_insert(e, drive, path);
}

/* wywoływane przez SDL, często z innego wątku - tylko przekazuje wynik do głównej pętli */
static void SDLCALL disk_dialog_done(void* userdata, const char* const* filelist, const int filter) {
    (void)filter;
    if (filelist == nullptr) {
        msg_printf(MSG_ERROR, "File dialog failed: %s\n", SDL_GetError());
        return;
    }
    if (filelist[0] == nullptr) {
        return; /* anulowano */
    }
    SDL_Event event;
    SDL_zero(event);
    event.type = DISK_EVENT_TYPE;
    event.user.code = (Sint32)(intptr_t)userdata;
    event.user.data1 = SDL_strdup(filelist[0]);
    if (event.user.data1 == nullptr || !SDL_PushEvent(&event)) {
        SDL_free(event.user.data1);
    }
}

bool disk_hotkey(struct elwro* e, const SDL_Keycode key, const SDL_Keymod mod) {
    if (key != SDLK_F5 && key != SDLK_F6) {
        return false;
    }
    const int drive = key == SDLK_F5 ? 0 : 1;
    if ((mod & SDL_KMOD_SHIFT) != 0) {
        disk_eject(e, drive);
    } else if ((mod & SDL_KMOD_CTRL) != 0) {
        disk_toggle_rw(e, drive);
    } else if (e->diskdir[0] != 0) {
        disk_cycle(e, drive);
    } else {
        /* początkowo katalog bieżący (z którego uruchomiono emulator), potem miejsce ostatnio wczytanej dyskietki */
        char* cwd = nullptr;
        const char* start = e->disk[drive]->path;
        if (start[0] == 0) {
            cwd = SDL_GetCurrentDirectory();
            start = cwd;
        }
        SDL_ShowOpenFileDialog(disk_dialog_done, (void*)(intptr_t)drive, e->scn->window,
            disk_filters, SDL_arraysize(disk_filters), start, false);
        SDL_free(cwd);
    }
    return true;
}

bool disk_event(struct elwro* e, const SDL_Event* event) {
    if (event->type != DISK_EVENT_TYPE) {
        return false;
    }
    char* path = event->user.data1;
    const int drive = event->user.code;
    if (path != nullptr) {
        disk_insert(e, drive, path);
        SDL_free(path);
    }
    return true;
}
