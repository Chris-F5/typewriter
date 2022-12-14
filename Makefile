CC=gcc
CFLAGS=-g

tw: tw.o error.o bytes.o stack.o parse.o interpret.o layout.o ttf.o pdf_content.o pdf.o
	$(CC) $^ -o $@

tw.o: tw.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

error.o: error.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

bytes.o: bytes.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

stack.o: stack.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

parse.o: parse.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

interpret.o: interpret.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

layout.o: layout.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

ttf.o: ttf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf_content.o: pdf_content.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@

pdf.o: pdf.c tw.h
	$(CC) $(CFLAGS) -c $< -o $@
