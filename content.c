#include "tw.h"

#include <stdlib.h>

static void write_pdf_string(FILE *stream, const char *string);

static void
write_pdf_string(FILE *stream, const char *string)
{
  char c;
  fprintf(stream, "(");
  while ( (c = *(string++)) ) {
    if (c == '(' || c == ')' || c == '\\')
      fputc('\\', stream);
    fputc(c, stream);
  }
  fprintf(stream, ")");
}

void
content_section_init(struct content_section *section, int width)
{
  section->width = width;
  section->line_spacing = 0;
  section->line_count = 0;
  section->line_allocated = 100;
  section->lines = xmalloc(
      sizeof(struct content_line) * section->line_allocated);
}

void
content_section_add_line(struct content_section *section,
    struct content_line line)
{
  int line_index;
  line_index = section->line_count;
  while (section->line_count >= section->line_allocated) {
    section->lines = xrealloc(section->lines,
        sizeof(struct content_line) * (section->line_allocated + 100));
  }
  section->line_count++;
  section->lines[line_index] = line;
}

void
content_section_draw(struct content_section *section, FILE *stream, int x,
    int y)
{
  int l;
  struct content_line *line;
  fprintf(stream, "0 0 0 rg");
  for (l = 0; l < section->line_count; l++) {
    line = &section->lines[l];
    y -= line->font_size;
    fprintf(stream, "\nBT ");
    fprintf(stream, "/F1 %d Tf ", line->font_size);
    fprintf(stream, "%d Tw ", line->word_spacing);
    fprintf(stream, "%d %d Td ", x, y);
    write_pdf_string(stream, line->text);
    fprintf(stream, "Tj ");
    fprintf(stream, "ET");
    y -= section->line_spacing;
  }
}

void
content_section_destroy(struct content_section *section)
{
  free(section->lines);
}
