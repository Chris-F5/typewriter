CC=gcc
CFLAGS=-g -Wall
LDFLAGS=

SRC = utils.c twpdf.c twwrite.c twpages.c twcontent.c twjpeg.c document.c stralloc.c
OBJ = $(SRC:.c=.o)

.PHONY: all

all: tw

tw: tw.o $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

config.h:
	cp config.def.h $@

.c.o:
	$(CC) $(CFLAGS) -c $<

tw.o: config.h utils.h twpdf.h document.h stralloc.h

utils.o: utils.h
twpdf.o: utils.h twpdf.h
twwrite.o: utils.h twpdf.h
twpages.o: utils.h twpdf.h twpages.h
twcontent.o: utils.h twpdf.h twcontent.h
twjpeg.o: utils.h twpdf.h twjpeg.h
document.o: utils.h twpdf.h twcontent.h twjpeg.h twpages.h document.h
stralloc.o: utils.h stralloc.h
