CC=gcc
CFLAGS=-g -Wall

tw: tw.o error.o dbuffer.o ttf.o pdf.o parse.o
	$(CC) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

dbuffer.o: dbuffer.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

parse.o: parse.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
