CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-lfontconfig

tw: tw.o utils.o dbuffer.o ttf.o
	$(CC) $(LDFLAGS) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

line_break.o: line_break.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

utils.o: utils.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

dbuffer.o: dbuffer.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
