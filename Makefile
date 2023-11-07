CC=gcc
CFLAGS=-g -Wall
LDFLAGS=

SRC = utils.c twpdf.c twwrite.c twpages.c twcontent.c twjpeg.c document.c stralloc.c
OBJ = $(SRC:.c=.o)
TARGETS = $(shell find . -type f -name 'tw-*.c' | sed 's/\.c$$//')

.PHONY: all

all: $(TARGETS)

$(TARGETS): tw-%: tw-%.o $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $<

tw-%.o: tw-%.c *.h
	$(CC) $(CFLAGS) -c $<

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $<

utils.o: utils.h
twpdf.o: utils.h twpdf.h
twwrite.o: utils.h twpdf.h
twpages.o: utils.h twpdf.h twpages.h
twcontent.o: utils.h twpdf.h twcontent.h
twjpeg.o: utils.h twpdf.h twjpeg.h
document.o: utils.h twpdf.h twcontent.h twjpeg.h twpages.h document.h
stralloc.o: utils.h stralloc.h
