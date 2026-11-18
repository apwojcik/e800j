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
#include "e800j_kbrd.h"
#include "e800j_msg.h"
#include "virt_fdd.h"
#include "zx_scrn.h"

/* SDL_EVENT_USER jest zajęte przez zegar CPU/ekranu (zx_scrn.c), a SDL_RegisterEvents() zwraca właśnie je */
#define DISK_EVENT_TYPE (SDL_EVENT_USER + 1)

/* w user.code: numer napędu, z DISK_NEW_FLAG dla nowej (pustej) dyskietki z okna zapisu pliku */
#define DISK_NEW_FLAG 0x10
#define DISK_BLANK_NAME "blank.dsk"

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

void disk_status(const struct elwro* e, const char* prefix, char* buf, const size_t size) {
    buf[0] = 0;
    size_t len = 0;
    for (int drive = 0; drive < 2 && len < size; drive++) {
        const struct vfdd_disk* disk = e->disk[drive];
        const char* sep = drive == 0 ? "" : ", ";
        const int n = disk->f == nullptr
            ? snprintf(buf + len, size - len, "%s%c: no disk", drive == 0 ? prefix : sep, drive_name(drive))
            : snprintf(buf + len, size - len, "%s%c: %s (%s)", drive == 0 ? prefix : sep, drive_name(drive),
                disk_basename(disk->path), (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
        len += (size_t)n;
    }
}

void disk_announce(struct elwro* e) {
    char text[256];
    disk_status(e, "Floppy ", text, sizeof(text));
    screen_toast(e->scn, "%s", text);
}

static void disk_insert_mode(struct elwro* e, const int drive, const char* path, const unsigned char mode) {
    struct vfdd_disk* disk = e->disk[drive];
    if (vfdd_swap(disk, path, mode) < 0) {
        msg_printf(MSG_ERROR, "Drive %c: opening file %s failed.\n", drive_name(drive), path);
        screen_toast(e->scn, "Floppy insert failed: %c: %s", drive_name(drive), disk_basename(path));
        return;
    }
    msg_printf(MSG_MSG, "Drive %c: loaded %s (%s)\n", drive_name(drive), path,
        (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
    screen_toast(e->scn, "Floppy inserted: %c: %s (%s)", drive_name(drive), disk_basename(path),
        (disk->stat & VFDD_WPROT) != 0 ? "RO" : "RW");
}

static void disk_insert(struct elwro* e, const int drive, const char* path) {
    disk_insert_mode(e, drive, path, e->disk[drive]->rw);
}

void disk_drop(struct elwro* e, const int drive, const char* path) {
    disk_insert(e, drive, path);
}

/* tworzy pusty (niesformatowany) obraz i wkłada go do napędu w trybie RW; plik formatuje dopiero system w emulowanym komputerze */
static void disk_insert_blank(struct elwro* e, const int drive, const char* chosen) {
    char path[1024];
    const char* base = disk_basename(chosen);
    const bool has_ext = strrchr(base, '.') != nullptr;
    snprintf(path, sizeof(path), "%s%s", chosen, has_ext ? "" : ".dsk");
    for (int i = 0; i < 2; i++) {
        if (e->disk[i]->f != nullptr && strcmp(e->disk[i]->path, path) == 0) {
            msg_printf(MSG_ERROR, "Blank disk %s is already inserted in drive %c:\n", path, drive_name(i));
            screen_toast(e->scn, "Blank disk not created: %s is inserted in %c:", disk_basename(path), drive_name(i));
            return;
        }
    }
    if (vfdd_create_image(path) < 0) {
        msg_printf(MSG_ERROR, "Creating blank disk image %s failed.\n", path);
        screen_toast(e->scn, "Blank disk creation failed: %s", disk_basename(path));
        return;
    }
    disk_insert_mode(e, drive, path, VFDD_RW);
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
    if ((mod & HK_WIN_MOD) != 0) {
        /* katalog: diskdir, w przeciwnym razie miejsce ostatnio wczytanej dyskietki, w przeciwnym razie bieżący */
        char start[1024];
        const char* path = e->disk[drive]->path;
        if (e->diskdir[0] != 0) {
            snprintf(start, sizeof(start), "%s/" DISK_BLANK_NAME, e->diskdir);
        } else if (path[0] != 0) {
            const size_t dirlen = (size_t)(disk_basename(path) - path);
            snprintf(start, sizeof(start), "%.*s" DISK_BLANK_NAME, (int)dirlen, path);
        } else {
            char* cwd = SDL_GetCurrentDirectory();
            snprintf(start, sizeof(start), "%s" DISK_BLANK_NAME, cwd != nullptr ? cwd : "");
            SDL_free(cwd);
        }
        SDL_ShowSaveFileDialog(disk_dialog_done, (void*)(intptr_t)(drive | DISK_NEW_FLAG), e->scn->window,
            disk_filters, SDL_arraysize(disk_filters), start);
    } else if ((mod & SDL_KMOD_SHIFT) != 0) {
        disk_eject(e, drive);
    } else if ((mod & HK_ALT_MOD) != 0) {
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
    const int drive = event->user.code & ~DISK_NEW_FLAG;
    if (path != nullptr) {
        if ((event->user.code & DISK_NEW_FLAG) != 0) {
            disk_insert_blank(e, drive, path);
        } else {
            disk_insert(e, drive, path);
        }
        SDL_free(path);
    }
    return true;
}
