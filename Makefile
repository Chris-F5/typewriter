CC=gcc
CFLAGS=-g

tw: tw.o ttf.o
	$(CC) $^ -o $@

tw.o: tw.c ttf.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c ttf.h
	$(CC) $(CFLAGS) -c $< -o $@
