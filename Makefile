

libs = e800j_main.o z80.o z80_debug.o e800j_kbrd.o e800j_file.o e800j_gui.o e800j_io.o e800j_mem.o\
       e800j_scrn.o i8251.o z80_instr.o

opts = -Wall -g

gtk_conf=`pkg-config --cflags gtk+-2.0`
gtk_libs=`pkg-config --libs gtk+-2.0`

ALL:elwro

elwro:$(libs)
	gcc -o elwro  $(libs) $(gtk_libs)

e800j_main.o: e800j_main.c e800j_prom.h e800j_file.h e800j.h e800j_kbrd.h e800j_main.h
	gcc -c $(opts) e800j_main.c

e800j_kbrd.o: e800j_kbrd.c e800j_kbrd.h
	gcc -c $(opts) e800j_kbrd.c

e800j_file.o: e800j_file.c e800j_file.h
	gcc -c $(opts) e800j_file.c

e800j_io.o: e800j_io.c e800j_io.h
	gcc -c $(opts) e800j_io.c

e800j_mem.o: e800j_mem.c e800j_mem.h
	gcc -c $(opts) e800j_mem.c

e800j_scrn.o: e800j_scrn.c e800j_scrn.h
	gcc -c $(opts) e800j_scrn.c

e800j_gui.o: e800j_gui.c e800j_gui.h
	gcc -c $(opts) $(gtk_conf) e800j_gui.c

i8251.o: i8251.c i8251.h
	gcc -c $(opts) i8251.c


z80.o: z80.h z80.c z80_instr.h z80_cpu.h
	gcc -c $(opts) z80.c

z80_instr.o: z80.h z80_instr.c z80_instr.h z80_cpu.h
	gcc -c $(opts) z80_instr.c


z80_debug.o: z80_debug.c z80.h z80.c z80_instr.h z80_cpu.h
	gcc -c $(opts) z80_debug.c
clean:
	rm -f *.o
	rm -f elwro



