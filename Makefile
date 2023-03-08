CC=gcc
CFLAGS=-g -Wall

all: tw line_break

tw: tw.o error.o dbuffer.o parse.o ttf.o pdf.o print_pages.o
	$(CC) $^ -o $@

line_break: line_break.o error.o dbuffer.o ttf.o
	$(CC) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

line_break.o: line_break.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

dbuffer.o: dbuffer.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

parse.o: parse.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

print_pages.o: print_pages.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
