
P_NAME = elwro
CC=gcc
#CFLAGS = -Wall -ggdb
CFLAGS = -Wall -O2

OBJS = z80_core.o e800j.o z80_debug.o zx_scrn.o e800j_kbrd.o e800j_file.o e800j_sound.o\
       e800j_mem.o e800j_io.o i8272_fdc.o e800j_fdc.o virt_fdd.o e800j_msg.o
LIBS = -lSDL -lGL -lspectrum -lm

#----------------------------------------------------------------------------
ALL:$(P_NAME)

$(P_NAME): $(OBJS)
	$(CC) -o $(P_NAME) $(OBJS) $(LIBS) $(GTK_LB)

zx_scrn.o: zx_scrn.c zx_scrn.h
	$(CC) -c $(CFLAGS) zx_scrn.c

z80_core.o: z80_core.c z80_core.h z80_cpu.h z80_dtbl.h
	$(CC) -c $(CFLAGS) z80_core.c

z80_debug.o: z80_debug.c z80_debug.h
	$(CC) -c $(CFLAGS) z80_debug.c

e800j_kbrd.o: e800j_kbrd.c e800j_kbrd.h
	$(CC) -c $(CFLAGS) e800j_kbrd.c

e800j_file.o: e800j_file.c e800j_file.h z80_cpu.h
	$(CC) -c $(CFLAGS) e800j_file.c

e800j_sound.o: e800j_sound.c e800j_sound.h
	$(CC) -c $(CFLAGS) e800j_sound.c

e800j.o: e800j.c zx_scrn.h z80_cpu.h
	$(CC) -c $(CFLAGS) e800j.c

e800j_mem.o: e800j_mem.c e800j_mem.h
	$(CC) -c $(CFLAGS) e800j_mem.c

e800j_io.o: e800j_io.c e800j_io.h
	$(CC) -c $(CFLAGS) e800j_io.c

e800j_fdc.o: e800j_fdc.c e800j_fdc.h
	$(CC) -c $(CFLAGS) e800j_fdc.c

i8272_fdc.o: i8272_fdc.c i8272_fdc.h
	$(CC) -c $(CFLAGS) i8272_fdc.c

virt_fdd.o: virt_fdd.c virt_fdd.h
	$(CC) -c $(CFLAGS) virt_fdd.c

e800j_msg.o: e800j_msg.c e800j_msg.h
	$(CC) -c $(CFLAGS) e800j_msg.c



clean:
	rm -f $(P_NAME)
	rm -f *.o
