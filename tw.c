#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ttf.h"

#define FNAME "fonts/cmu.serif-roman.ttf"

int
main()
{
  int ret = 0;
  FILE *file = NULL;
  long size;
  char *ttf = NULL;
  struct font_info info;

  file = fopen(FNAME, "r");
  if (file == NULL) {
    char *err = strerror(errno);
    fprintf(stderr, "failed to open font file '%s': %s\n", FNAME, err);
    goto cleanup;
  }

  errno = 0;
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
  if (errno) {
    perror("failed to get length of file");
    goto cleanup;
  }

  ttf = malloc(size);
  if (ttf == NULL) {
    perror("malloc failed");
    goto cleanup;
  }

  if (fread(ttf, 1, size, file) != size) {
    fprintf(stderr, "failed to read all bytes from file\n");
    goto cleanup;
  }

  if (read_ttf(ttf, size, &info) != 0) {
    fprintf(stderr, "failed to parse ttf\n");
    goto cleanup;
  }

  for(int i = 0; i < 256; i++)
    printf("%d ", info.char_widths[i]);
  printf("\n");

cleanup:
  free(ttf);
  fclose(file);
  return ret;
}
