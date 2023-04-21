CC=gcc
CFLAGS=-g -Wall

all: tw line_break

tw: tw.o utils.o dbuffer.o record.o ttf.o jpeg.o pdf.o
	$(CC) $^ -o $@

line_break: line_break.o utils.o dbuffer.o record.o ttf.o
	$(CC) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

line_break.o: line_break.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

utils.o: utils.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

dbuffer.o: dbuffer.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

record.o: record.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

jpeg.o: jpeg.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
