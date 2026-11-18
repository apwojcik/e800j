# ToDo investigations

Notes from going through `doc/ToDo` (Krzysztof Komarnicki, 2006). Status as of 2026-11-18.

| ToDo item | Status |
|---|---|
| Add the FORMAT command to the FDC | Implemented (commit "fdc: implement FORMAT A TRACK ...") |
| Handle RST #48 under CP/J | Verified, no emulator change needed, item removed |
| GUI | Partly done: error message boxes, drag and drop, fullscreen/pause keys and PNG screenshot are implemented; the rest is open (see section 3) |

---

## 1. FORMAT command in the FDC

### State before

`FORMAT A TRACK` (opcode 0x0D) was in the command table (`i8272_fdc.h`, 6 command bytes, 7 result bytes) but its
handler `i8272_ft_cmd` only logged "Unsupported command". `virt_fdd.c` could seek to existing tracks and
read/write bytes inside existing sectors, but could not create or rewrite a track.

### What the chip needs

- Command phase: opcode, HD/US, N, SC, GPL, D.
- Execute phase: the CPU sends SC x 4 bytes (C, H, R, N); the controller writes the ID fields and fills the data
  fields with D. No TC is used, the command ends by itself after the last sector.
- Result phase: ST0, ST1, ST2, C, H, R, N.

### Implementation

- `i8272_ft_cmd` (`i8272_fdc.c`) is a small state machine like the read/write commands. The IDs are kept in
  `fmt_id[256 * 4]` of `struct i8272`. TC during formatting is ignored. Errors use the same status bits as
  write: no disk, write protected (NW), bad geometry (ND).
- New drive action `FDD_FORMAT_TRACK`; `vfdd_drive` passes `pcn` (physical track), `hds`, N, SC, GPL, D and the
  ID table to `vfdd_format_track()`.
- `vfdd_format_track()` (`virt_fdd.c`) builds the whole track block (0x100 header with sector list + data) in
  memory and splices it into the CPCEmu `.dsk` file:
  - existing track: replaced; new track: inserted in track/head order (keeps the image valid for other DSK
    tools);
  - if the size changes, later tracks are shifted and the file is truncated (`_chsize_s` / `ftruncate`) or grown;
  - the image header (track count, head count, max track size) is updated; an empty file gets a fresh header.
- Limits come from the DSK format: N <= 6 and at most 29 sectors per track (sector list must fit in the track
  header). Anything above is reported as sector-not-found (ST1 ND).

### Verification

A throwaway harness (MSVC) formatted an empty image, then inserted a track in the middle, replaced it with a
smaller layout (file shrank) and a larger one (file grew). Neighbouring tracks kept their data, the final size
was exact, and a write-protected disk returned `FDD_ERR_WPROT`. Not yet exercised from inside the emulator with a
CP/J format utility.

### Related

- Blank disks: `vfdd_create_image()` writes a header-only (unformatted) `.dsk`. Used by Win+F5/F6 (save dialog)
  and by `-A` / `-B` when the file does not exist; the guest formats it with FORMAT A TRACK.
- Side finding: `vfdd_seek_sector` ignores the return value of `vfdd_seek_track`, so a missing track leaves a
  stale track header in use. `i8272_seek_cmd` sets `pcn` but `FDD_SEEK` seeks by `fdc->cn`. Both harmless so far
  because read/write seek by sector themselves.

---

## 2. RST #48 under CP/J

### Meaning of the item

0x48 is not a Z80 restart vector (valid: 00, 08, 10, 18, 20, 28, 30, 38). "#48" is decimal 48 = 0x30, i.e.
**RST 30h** (Intel 8080 name: RST 6). The author's disassembly (`doc/junior_boot.asm`) also prints RST operands in
decimal (`RST 56` = 0x38).

CP/J is the Elwro 800 Junior implementation of CP/M. CP/M programs call BDOS with `CALL 5`, so the RST vectors
stay free. Debuggers such as DDT use RST 38h for breakpoints, which collides with the IM 1 interrupt (also 0x38);
debuggers like DDTZ / Z8E use RST 30h instead.

### What the emulator does

- `z80_rst` (`z80_core.c`) handles all eight RST opcodes the same way; nothing is special about 30h.
- Memory paging changes only through port 0xF7 (`e800j_io.c`, `e800j_mem.c`). Nothing pages memory based on an
  opcode fetch.
- From the PROM table, only `mem_mask = 0x180` (port F7 = 0xA0) maps page 0 to RAM; all other values put BAS0 or
  BOOT ROM at 0x0000. In the ROM configurations `RST 30h` runs into the BOOT ROM's `RST 38h` -> `RET`
  (`junior_boot.asm`, 0x0030-0x0038), so a breakpoint would do nothing there. Writes always go to RAM, so a
  debugger can still plant its `JP` at 0x30 and it takes effect once CP/J maps all RAM.

### Verification

Run in the emulator debugger (`--debug`) with all-RAM paging:

```
o 0xf7 0xa0
a 0x30 0xc3,0x00,0x02      ; JP 0200h at the RST 30h vector
a 0x0 0xf7,0x76            ; RST 30h ; HALT
a 0x200 0x3e,0x42,0xc9     ; LD A,42h ; RET
t 4 1
```

Trace: `RST $30` -> `JP $0200` -> `LD A,$42` -> `RET` back to `PC=0001`, `A=42`, stack restored. So RST 30h
works as expected and the item was removed from `doc/ToDo`.

### Debugger fixes found on the way

- `r ix=`, `iy=`, `sp=`, `pc=`, `i=`, `r=` all wrote to register A (copy-paste bug in `dbg_setreg`). Fixed.
- On Windows the GUI-subsystem exe fought the parent shell for the console. `--debug` (and the new F11 hot key)
  now open a dedicated console window, closed again on `q`; redirected stdin/stdout are kept; EOF on the
  debugger input quits it; `g` with nothing to wait for turns the per-instruction callbacks off.

---

## 3. GUI (partly done)

### What exists

- SDL3 window with a legacy OpenGL texture, nearest-neighbour scaling, resize, scale presets (Ctrl+F1..F5,
  x1..x3), `-f` fullscreen and `-n` no-frame at startup, per-scanline border.
- Overlays drawn with SDL's debug font into GL textures: F1 hot-key help, operation toasts (`osd_time`), volume
  bar, "PAUSED" indicator.
- Native file open / save dialogs for disks (F5/F6, Win+F5/F6) and snapshots (Win+F12).
- Hot keys for disks, volume, snapshots, resets, screenshot (F8 / Ctrl+F8), fullscreen (F9), pause (Pause) and the
  debugger (F11); Elwro keys on Home, End, PgUp, PgDn, F10. Files dropped on the window are loaded.
- Critical startup errors are shown in a message box when the program has no terminal.
- Leftovers of an older design: `struct screen` still has `parent` (GTK window ID) and `yresv` (strip reserved
  for a GTK menu), so the 2006 version apparently embedded the screen in a GTK frontend with a menu.

### Gaps still open (checked in the code)

- No mouse, gamepad or speed (turbo / slow) control; the window title never shows disk or speed status.
- No focus-loss pause.
- No scaling filter, aspect ratio, integer scaling or vsync options.
- No menus, settings screen or on-screen keyboard; the debugger is console only.
- libspectrum is linked but its tape support is unused.

### Options by effort

Small:
1. ~~Fatal startup errors in a message box~~ done (`msg_dialog`, `SDL_ShowSimpleMessageBox`).
2. ~~Drag and drop of `.dsk` and snapshots~~ done: `.dsk` to drive A (left half of the window) or B (right half),
   a second `.dsk` of the same drop to the other drive, snapshots loaded like Win+F12.
3. ~~Fullscreen hot key and pause key~~ done (F9, Pause).
4. Window title with disk names and RO/RW.
5. ~~PNG screenshot~~ done (F8 with border 352x288, Ctrl+F8 without 256x192).
6. Drive activity LED during FDC access.

Medium:
7. On-screen keyboard overlay. Probably the most valuable: the Elwro layout and E mode (Ctrl, then Alt+digit) are
   hard to learn (`dsk/load.txt` has to explain it), and it would bring mouse input.
8. Settings overlay in the existing style (volume, scale, filter, brightness, speed, floppies), written back to
   `e800j.cfg`.
9. Speed control (turbo while a key is held, slow motion); easy because timing follows CPU tick counts.

Large:
10. A real menu bar. SDL3 has no native menus: per-platform code (Win32, Cocoa, GTK), or embed Dear ImGui over the
    existing GL context (menus, config dialog, disk manager, memory and debugger views in one dependency).
11. GUI debugger (registers, disassembly, memory, breakpoints), building on the F11 / `dbg_break` work.

### Recommendation

Items 1, 2, 3 and 5 are done. Next: 4 and 6 (cheap), then choose between 7 (on-screen keyboard) and ImGui
(covers 8 and 11 as well): ImGui for a "proper" GUI, the overlay approach to keep the project dependency-light.
