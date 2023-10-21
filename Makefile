CC=gcc
CFLAGS=-g -Wall
LDFLAGS=

tw: tw.o twpdf.o twwrite.o utils.o
	$(CC) $(LDFLAGS) $^ -o $@

tw.o: tw.c twpdf.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

twpdf.o: twpdf.c twpdf.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

twwrite.o: twwrite.c twpdf.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@
