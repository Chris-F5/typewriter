CC=gcc
CFLAGS=-g

tw: tw.o ttf.o pdf.o
	$(CC) $^ -o $@

tw.o: tw.c ttf.h utils.h pdf.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c pdf.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c ttf.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@
