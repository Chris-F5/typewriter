#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tw.h"

struct style {
  int font_size;
};

struct span {
  struct style style;
  struct dbuffer buffer;
  struct span *next;
};

struct slug {
  int width;
  struct span *spans;
  struct span *last_span;
};

struct opt_break {
  struct slug at_break;
  struct slug no_break;
};

struct line {
  int target_width, width, height, spaces;
  int in_string;
  struct style style;
  struct dbuffer buffer;
};

static void escape_char(struct dbuffer *buffer, unsigned char c);
static void slug_free(struct slug *slug);
static void line_init(struct line *line, int target_width);
static void line_reset(struct line *line, int target_width);
static void line_free(struct line *line);
static void line_close_string(struct line *line);
static void line_open_string(struct line *line);
static void line_change_style(struct line *line, const struct style *new_style);
static void line_add_span(struct line *line, const struct span *span);
static void line_add_slug(struct line *line, const struct slug *slug);
static void line_end(struct line *line);
static void line_print(FILE *output, const struct line *line);
static void fit_slug(FILE *output, struct line *line, const struct slug *slug,
    struct opt_break *end_break, struct opt_break this_break);
static int parse_style(FILE *input, FILE *output, struct line *line,
    struct slug *slug, struct style *style);
static int parse_span(FILE *input, FILE *output, struct line *line,
    struct slug *slug, struct style *style, const struct font_info *font_info);
static void char_slug(struct slug *slug, char c, const struct style *style,
    const struct font_info *font_info);
static int parse_break(FILE *input, FILE *output, struct line *line,
    struct slug *slug, struct opt_break *end_break, struct style *style,
    const struct font_info *font_info);
static void new_line(FILE *output, struct line *line, struct slug *slug,
    struct opt_break *end_break);
static int line_break(FILE *input, FILE *output, int target_width,
    const struct font_info *font_info);

const struct slug empty_slug = { 0, NULL };
const struct opt_break empty_break = { empty_slug, empty_slug };

const struct style default_style = {
  0,
};

static void
escape_char(struct dbuffer *buffer, unsigned char c)
{
  if (c < 32 || c > 127)
    return;
  switch (c) {
    case '(':
    case ')':
    case '\\':
      dbuffer_putc(buffer, '\\');
    default:
      dbuffer_putc(buffer, c);
  }
}

static void
slug_free(struct slug *slug)
{
  struct span *span;
  struct span *next_span;
  for (span = slug->spans; span; span = next_span) {
    next_span = span->next;
    dbuffer_free(&span->buffer);
    free(span);
  }
}

static void
line_init(struct line *line, int target_width)
{
  dbuffer_init(&line->buffer, 1024 * 4, 1024 * 4);
  line_reset(line, target_width);
}

static void
line_reset(struct line *line, int target_width)
{
  line->target_width = target_width;
  line->height = 0;
  line->width = 0;
  line->spaces = 0;
  line->in_string = 0;
  line->style = default_style;
  line->buffer.size = 0;
}

static void
line_free(struct line *line)
{
  dbuffer_free(&line->buffer);
}

static void
line_close_string(struct line *line)
{
  if (line->in_string) {
    dbuffer_printf(&line->buffer, ") Tj ");
    line->in_string = 0;
  }
}

static void
line_open_string(struct line *line)
{
  if (!line->in_string) {
    dbuffer_printf(&line->buffer, "(");
    line->in_string = 1;
  }
}

static void
line_change_style(struct line *line, const struct style *new_style)
{
  if (line->style.font_size == new_style->font_size)
    return;
  line_close_string(line);
  if (line->style.font_size != new_style->font_size)
    dbuffer_printf(&line->buffer, "/Regular %d Tf ", new_style->font_size);
  line->style = *new_style;
}

static void
line_add_span(struct line *line, const struct span *span)
{
  int i;
  line_change_style(line, &span->style);
  line_open_string(line);
  for (i = 0; i < span->buffer.size; i++) {
    if (span->buffer.data[i] == ' ')
      line->spaces++;
    escape_char(&line->buffer, span->buffer.data[i]);
  }
  if (line->height < span->style.font_size)
    line->height = span->style.font_size;
}

static void
line_add_slug(struct line *line, const struct slug *slug)
{
  const struct span *span;
  line->width += slug->width;
  for (span = slug->spans; span; span = span->next)
    line_add_span(line, span);
}

static void
line_end(struct line *line)
{
  line_close_string(line);
}

static void
line_print(FILE *output, const struct line *line)
{
  fprintf(output, "graphic %d %d\n", line->width / 1000, line->height);
  fprintf(output, "TEXT ");
  fwrite(line->buffer.data, 1, line->buffer.size, output);
  fputc('\n', output);
  fprintf(output, "endgraphic\n");
}

static void
fit_slug(FILE *output, struct line *line, const struct slug *slug,
    struct opt_break *end_break, struct opt_break this_break)
{
  int slack;
  slack = line->target_width - line->width - end_break->no_break.width;
  if (line->width == 0 || slug->width + this_break.at_break.width <= slack) {
    line_add_slug(line, &end_break->no_break);
  } else {
    line_add_slug(line, &end_break->at_break);
    line_end(line);
    line_print(output, line);
    line_reset(line, line->target_width);
  }
  line_add_slug(line, slug);
  slug_free(&end_break->at_break);
  slug_free(&end_break->no_break);
  *end_break = this_break;
}

static int
parse_style(FILE *input, FILE *output, struct line *line, struct slug *slug,
    struct style *style)
{
  int scan, n;
  char attrib[5];
  scan = fscanf(input, "%4s", attrib);
  if (scan < 0 || scan == EOF)
    goto error;
  if (strcmp("SIZE", attrib) == 0) {
    scan = fscanf(input, " %d", &n);
    if (scan < 0 || scan == EOF)
      goto error;
    style->font_size = n;
  } else {
    goto error;
  }
  return 0;
error:
  fprintf(stderr, "Failed to parse style command options.");
  return 1;
}

static int
parse_span(FILE *input, FILE *output, struct line *line, struct slug *slug,
    struct style *style, const struct font_info *font_info)
{
  int c, span_width;
  struct span *span;
  if (slug->spans == NULL)
    span = slug->spans = slug->last_span = xmalloc(sizeof(struct span));
  else
    span = slug->last_span = slug->last_span->next = xmalloc(sizeof(struct span));
  span->style = *style;
  span->next = NULL;
  dbuffer_init(&span->buffer, 128, 128);
  span_width = 0;
  while ( (c = fgetc(input)) != EOF) {
    if (c == '\n')
      break;
    dbuffer_putc(&span->buffer, (char)c);
    span_width += font_info->char_widths[c];
  }
  span_width = span_width * span->style.font_size;
  slug->width += span_width;
  return 0;
}

static void
char_slug(struct slug *slug, char c, const struct style *style,
    const struct font_info *font_info)
{
  slug->spans = slug->last_span = xmalloc(sizeof(struct span));
  slug->last_span->style = *style;
  dbuffer_init(&slug->last_span->buffer, 2, 1024);
  dbuffer_putc(&slug->last_span->buffer, c);
  slug->last_span->next = NULL;
  slug->width = font_info->char_widths[(unsigned char)c]
    * style->font_size;
}

static int
parse_break(FILE *input, FILE *output, struct line *line, struct slug *slug,
    struct opt_break *end_break, struct style *style,
    const struct font_info *font_info)
{
  int scan;
  unsigned char no_break_char, at_break_char;
  struct opt_break this_break;
  scan = fscanf(input, "%c%c", &no_break_char, &at_break_char);
  if (scan < 0 || scan == EOF) {
    fprintf(stderr, "Failed to parse line break command options.\n");
    return 1;
  }
  if (no_break_char == '%')
    this_break.no_break = empty_slug;
  else
    char_slug(&this_break.no_break, no_break_char, style, font_info);
  if (at_break_char == '%')
    this_break.at_break = empty_slug;
  else
    char_slug(&this_break.at_break, at_break_char, style, font_info);
  fit_slug(output, line, slug, end_break, this_break);
  slug_free(slug);
  slug->width = 0;
  slug->spans = slug->last_span = NULL;
  return 0;
}

static void
new_line(FILE *output, struct line *line, struct slug *slug,
    struct opt_break *end_break)
{
  if (slug->width)
    fit_slug(output, line, slug, end_break, empty_break);
  *end_break = empty_break;
  slug_free(slug);
  slug->width = 0;
  slug->spans = slug->last_span = NULL;
  line_end(line);
  if (line->width)
    line_print(output, line);
  line_reset(line, line->target_width);
}

static int
line_break(FILE *input, FILE *output, int target_width,
    const struct font_info *font_info)
{
  int c;
  struct line line;
  struct slug slug;
  struct style style;
  struct opt_break end_break;
  line_init(&line, target_width);
  slug.width = 0;
  slug.spans = slug.last_span = NULL;
  style = default_style;
  end_break = empty_break;
  while ( (c = fgetc(input)) != EOF) {
    switch (c) {
    case '\n':
      break;
    case '#':
      parse_style(input, output, &line, &slug, &style);
      break;
    case '^':
      parse_span(input, output, &line, &slug, &style, font_info);
      break;
    case '/':
      parse_break(input, output, &line, &slug, &end_break, &style, font_info);
      break;
    case ';':
      new_line(output, &line, &slug, &end_break);
      break;
    default:
      fprintf(stderr, "Unknown command char '%c'\n", (char)c);
      break;
    }
  }
  new_line(output, &line, &slug, &end_break);
  line_free(&line);
  slug_free(&slug);
  return 0;
}

int
main(int argc, char **argv)
{
  FILE *font_file, *input_file, *output_file;
  struct font_info font_info;
  input_file = stdin;
  output_file = stdout;
  font_file = fopen("./cmu.serif-roman.ttf", "rb");
  read_ttf(font_file, &font_info);

  line_break(input_file, output_file, 595 * 1000, &font_info);

  fclose(font_file);
  return 0;
}
