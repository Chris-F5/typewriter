CC=gcc
CFLAGS=-g

tw: tw.o error.o stack.o parse.o style.o layout.o ttf.o pdf.o content.o
	$(CC) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

stack.o: stack.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

parse.o: parse.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

style.o: style.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

layout.o: layout.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
