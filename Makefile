CC=gcc
CFLAGS=-g

tw: tw.o ttf.o pdf.o content.o
	$(CC) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

content.o: content.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
