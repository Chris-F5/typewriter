#include "content.h"

#include <stdlib.h>

int
content_section_init(struct content_section *section, int width)
{
  section->error_flags = 0;
  section->width = width;
  section->line_spacing = 0;
  section->line_count = 0;
  section->line_allocated = 100;
  section->lines = malloc(
      sizeof(struct content_line) * section->line_allocated);
  if (section->lines == NULL) {
    section->line_allocated = 0;
    section->error_flags |= CONTENT_ERROR_FLAG_MEMORY;
  }
  return section->error_flags;
}

int
content_section_add_line(struct content_section *section,
    struct content_line line)
{
  int line_index;
  line_index = section->line_count;
  while (section->line_count >= section->line_allocated) {
    section->lines = realloc(section->lines,
        sizeof(struct content_line) * (section->line_allocated + 100));
    if (section->lines) {
      section->line_allocated += 100;
    } else {
      section->error_flags |= CONTENT_ERROR_FLAG_MEMORY;
      return CONTENT_ERROR_FLAG_MEMORY;
    }
  }
  section->line_count++;
  section->lines[line_index] = line;
  return 0;
}

int
content_section_draw(struct content_section *section, FILE *stream, int x,
    int y)
{
  int l;
  struct content_line *line;
  
  fprintf(stream, "0 0 0 rg");

  for (l = 0; l < section->line_count; l++) {
    line = &section->lines[l];
    y -= line->font_size;
    fprintf(stream, "\nBT /F1 %d Tf %d %d Td (%s)Tj ET", line->font_size, x, y,
        line->text);
    y -= section->line_spacing;
  }

  return 0;
}

int
content_section_destroy(struct content_section *section)
{
  free(section->lines);
  return section->error_flags;
}
