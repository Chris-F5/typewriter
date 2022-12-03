#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ttf.h"
#include "utils.h"

int read_font_file(const char *fname, struct font_info *info);

int
read_font_file(const char *fname, struct font_info *info)
{
  int ret = 0;
  long size;
  FILE *file = NULL;
  char *ttf = NULL;

  (file = fopen(fname, "r")) OR goto file_error;
  fseek(file, 0, SEEK_END) == 0 OR goto file_error;
  (size = ftell(file)) >= 0 OR goto file_error;
  fseek(file, 0, SEEK_SET) == 0 OR goto file_error;
  (ttf = malloc(size)) OR goto allocation_error;
  fread(ttf, 1, size, file) == size OR goto file_error;
  read_ttf(ttf, size, info) == 0 OR goto ttf_error;
cleanup:
  free(ttf);
  if (file != NULL)
    fclose(file);
  return ret;
file_error:
  fprintf(stderr, "file error in '%s': %s\n", fname, strerror(errno));
  ret = 1;
  goto cleanup;
allocation_error:
  perror("failed to allocate memory");
  ret = 1;
  goto cleanup;
ttf_error:
  fprintf(stderr, "failed to parse ttf '%s'\n", fname);
  ret = 1;
  goto cleanup;
}

int
main()
{
  struct font_info font_info;

  read_font_file("fonts/cmu.serif-roman.ttf", &font_info) == 0 OR return 1;

  printf("font x_min: %d\n", font_info.x_min);
  printf("font y_min: %d\n", font_info.y_min);
  printf("font x_max: %d\n", font_info.x_max);
  printf("font y_max: %d\n", font_info.y_max);

  return 0;
}
