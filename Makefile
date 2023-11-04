CC=gcc
CFLAGS=-g -Wall
LDFLAGS=

tw: tw.o utils.o twpdf.o twwrite.o twpages.o twcontent.o twjpeg.o stralloc.o
	$(CC) $(LDFLAGS) $^ -o $@

config.h:
	cp config.def.h $@

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@

tw.o: tw.c config.h utils.h twpdf.h
	$(CC) $(CFLAGS) -c $< -o $@

twpdf.o: twpdf.c utils.h twpdf.h
	$(CC) $(CFLAGS) -c $< -o $@

twwrite.o: twwrite.c utils.h twpdf.h
	$(CC) $(CFLAGS) -c $< -o $@

twpages.o: twpages.c utils.h twpdf.h twpages.h
	$(CC) $(CFLAGS) -c $< -o $@

twcontent.o: twcontent.c utils.h twpdf.h twcontent.h
	$(CC) $(CFLAGS) -c $< -o $@

twjpeg.o: twjpeg.c utils.h twpdf.h twjpeg.h
	$(CC) $(CFLAGS) -c $< -o $@

stralloc.o: stralloc.c utils.h stralloc.h
	$(CC) $(CFLAGS) -c $< -o $@
