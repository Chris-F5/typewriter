#ifndef TW_CONTENT_H
#define TW_CONTENT_H

#define CONTENT_ERROR_FLAG_MEMORY 0b1

#include <stdio.h>

struct content_line {
  int font_size;
  char *text;
};

struct content_section {
  int error_flags;
  int width;
  int line_spacing;
  int line_count;
  int line_allocated;
  struct content_line *lines;
};

int content_section_init(struct content_section *section, int width);
int content_section_add_line(struct content_section *section,
    struct content_line line);
int content_section_draw(struct content_section *section, FILE *stream, int x,
    int y);
int content_section_destroy(struct content_section *section);

#endif
