
COMP_CONFIG =./Make.config

include $(COMP_CONFIG)

ifeq ($(LIB_SDL),yes)
SDL_LIB := -lSDL
SDL_DEF ?=
else
SDL_DEF ?=
SDL_DEF := -DNO_SDL
endif

ifeq ($(LIB_SPECTRUM),yes)
LIBSPECTRUM_LIB := -lspectrum
LIBSPECTRUM_DEF := -DUSE_LIBSPECTRUM
else
LIBSPECTRUM_LIB ?=
LIBSPECTRUM_DEF ?=
endif

ifeq ($(DEBUG_GDB),yes)
OPTION   = $(ALL_OPTION) -g
else
OPTION   = $(ALL_OPTION) $(OPTIMALIZATION)
endif

ifeq ($(PROFILER),yes)
OPTION   = $(ALL_OPTION) -g -pg
PROFILER_LIB = -pg
endif

ifeq ($(LIB_GTK20),yes)
GTK_FLG =`pkg-config --cflags gtk+-2.0`
GTK_LIB =`pkg-config --libs gtk+-2.0`
else
GTK_FLG =`gtk-config --cflags`
GTK_LIB =`gtk-config --libs`
LIB_GTK12= -DUSE_GTK12 
endif

ifeq ($(Z80_DEBUGGER),yes)
Z80_DEBUGGER_DEF = -DDEBUGGER
Z80_DEBUGGER_LIB = z80_debug.o
else
Z80_DEBUGGER_DEF ?=
Z80_DEBUGGER_LIB ?=
endif

ifeq ($(RUN_SNAPSHOT),yes)
RUN_DEF ?= -DRUN_AT_START
else
RUN_DEF ?=
endif

ifeq ($(WIN32_SUPPORT),yes)
WIN32_DEF = -DWIN32
else
WIN32_DEF ?=
endif

OPTION := $(OPTION) $(WIN32_DEF)

OBJS = e800j_main.o z80.o e800j_kbrd.o e800j_file.o e800j_gui.o e800j_io.o e800j_mem.o\
       e800j_scrn.o i8251.o z80_instr.o i8272.o e800j_debug.o $(Z80_DEBUGGER_LIB)

ALL:$(EMULATOR_NAME)

$(EMULATOR_NAME):$(OBJS) 
	$(CC) -o $(EMULATOR_NAME) $(OBJS) $(GTK_LIB) $(LIBSPECTRUM_LIB) $(SDL_LIB) $(PROFILER_LIB)

e800j_main.o: e800j_main.c e800j_prom.h e800j_file.h e800j.h e800j_kbrd.h e800j_main.h i8272.h
	$(CC) -c $(OPTION) e800j_main.c

%.o: %.c %.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) $<

z80.o: z80.h z80.c z80_instr.h z80_cpu.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) z80.c $(Z80_DEBUGGER_DEF)

z80_instr.o: z80.h z80_instr.c z80_instr.h z80_cpu.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) z80_instr.c $(Z80_DEBUGGER_DEF)

z80_debug.o: z80_debug.c z80.h z80.c z80_instr.h z80_cpu.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) z80_debug.c $(Z80_DEBUGGER_DEF)

e800j_file.o: e800j_file.c e800j_file.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) e800j_file.c $(LIBSPECTRUM_DEF) $(RUN_DEF)

e800j_gui.o: e800j_gui.c e800j_gui.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) e800j_gui.c $(SDL_DEF)  $(GTK_FLG) $(LIB_GTK_12) $(Z80_DEBUGGER_DEF)

e800j_debug.o: e800j_debug.c e800j_debug.h $(COMP_CONFIG)
	$(CC) -c $(OPTION) e800j_debug.c $(SDL_DEF)  $(GTK_FLG) $(LIB_GTK_12) $(Z80_DEBUGGER_DEF)

clean:
	rm -f *.o
	rm -f $(EMULATOR_NAME)

