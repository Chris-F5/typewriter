/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "tw.h"

enum align {
  ALIGN_LEFT,
  ALIGN_RIGHT,
  ALIGN_CENTRE,
  ALIGN_JUSTIFIED,
};

enum gizmo_type {
  GIZMO_TEXT,
  GIZMO_BREAK,
  GIZMO_MARK,
};

struct typeface {
  char font_name[256];
  struct font_info font_info;
  struct typeface *next;
};

struct style {
  int font_size;
  char font_name[256];
};

struct gizmo {
  int type;
  struct gizmo *next;
  char _[];
};

struct text_gizmo {
  int type; /* GIZMO_TEXT */
  struct gizmo *next;
  int width;
  struct style style;
  char string[];
};

struct break_gizmo {
  int type; /* GIZMO_BREAK */
  struct gizmo *next;
  int does_break, early_break, total_penalty, spacing;
  struct break_gizmo *best_source;
  struct style style;
  int no_break_width, at_break_width;
  char *no_break, *at_break;
  char strings[];
};

struct mark_gizmo {
  int type; /* GIZMO_MARK */
  struct gizmo *next;
  char string[];
};

static struct typeface *open_typeface(FILE *typeface_file);
static void free_typeface(struct typeface *typeface);
static int get_text_width(const char *string, const struct style *style,
    const struct typeface *typeface);
static struct gizmo *parse_gizmos(FILE *file, const struct typeface *typeface);
static void free_gizmos(struct gizmo *gizmo);
static void consider_breaks(struct gizmo *gizmo, int source_penalty,
    struct break_gizmo *source, int line_width);
static void optimise_breaks(struct gizmo *gizmo, int line_width);
static void print_text(struct dbuffer *buffer, const struct style *old_style,
    const struct style *style, const char *string, int *spaces);
static void print_gizmos(FILE *output, struct gizmo *gizmo, int line_width,
    int align);

static struct typeface *
open_typeface(FILE *typeface_file)
{
  struct typeface *first_font;
  struct typeface **next_font;
  int parse_result;
  struct record record;
  FILE *font_file;
  first_font = NULL;
  next_font = &first_font;
  init_record(&record);
  for (;;) {
    parse_result = parse_record(typeface_file, &record);
    if (parse_result == EOF)
      break;
    if (parse_result)
      continue;
    if (record.field_count != 2) {
      fprintf(stderr, "Typeface records must have exactly 2 fields.");
      continue;
    }
    if (strlen(record.fields[0]) >= 256) {
      fprintf(stderr,
          "Typeface file contains font name that is too long '%s'.\n",
          record.fields[0]);
      continue;
    }
    if (!is_font_name_valid(record.fields[0])) {
      fprintf(stderr, "Typeface file contains invalid font name '%s'.\n",
          record.fields[0]);
      continue;
    }
    font_file = fopen(record.fields[1], "r");
    if (font_file == NULL) {
      fprintf(stderr, "Failed to open ttf file '%s': %s\n",
          record.fields[1], strerror(errno));
      continue;
    }
    *next_font = malloc(sizeof(struct typeface));
    strcpy((*next_font)->font_name, record.fields[0]);
    if (read_ttf(font_file, &(*next_font)->font_info)) {
      fprintf(stderr, "Failed to parse ttf file: '%s'\n", record.fields[1]);
      free(*next_font);
      *next_font = NULL;
    }
    next_font = &(*next_font)->next;
    fclose(font_file);
  }
  free_record(&record);
  return first_font;
}

static void
free_typeface(struct typeface *typeface)
{
  struct typeface *next;
  while (typeface) {
    next = typeface->next;
    free(typeface);
    typeface = next;
  }
}

static int
get_text_width(const char *string, const struct style *style,
    const struct typeface *typeface)
{
  int width;
  const struct typeface *font;
  font = typeface;
  while (font && strcmp(font->font_name, style->font_name))
    font = font->next;
  if (font == NULL) {
    fprintf(stderr, "Typeface does not include font: '%s'\n", style->font_name);
    font = typeface;
  }
  width = 0;
  for (; *string; string++)
    width += font->font_info.char_widths[(unsigned char)*string];
  return width * style->font_size;
}

static struct gizmo *
parse_gizmos(FILE *file, const struct typeface *typeface)
{
  struct gizmo *first_gizmo;
  struct gizmo **next_gizmo;
  struct style current_style;
  int parse_result, arg1;
  struct record record;
  first_gizmo = NULL;
  next_gizmo = &first_gizmo;
  current_style.font_name[0] = '\0';
  current_style.font_size = 0;
  init_record(&record);
  for (;;) {
    parse_result = parse_record(file, &record);
    if (parse_result == EOF)
      break;
    if (parse_result)
      continue;
    if (strcmp(record.fields[0], "STRING") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Text STRING command must have 1 option.\n");
        continue;
      }
      if (current_style.font_name[0] == '\0') {
        fprintf(stderr,
            "Text STRING command can't be called without FONT set.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct text_gizmo)
          + strlen(record.fields[1]) + 1);
      (*next_gizmo)->type = GIZMO_TEXT;
      (*next_gizmo)->next = NULL;
      ((struct text_gizmo *)*next_gizmo)->width
        = get_text_width(record.fields[1], &current_style, typeface);
      ((struct text_gizmo *)*next_gizmo)->style = current_style;
      strcpy(((struct text_gizmo *)*next_gizmo)->string, record.fields[1]);
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
    if (strcmp(record.fields[0], "FONT") == 0) {
      if (record.field_count != 3) {
        fprintf(stderr, "Text FONT command must have 2 options.\n");
        continue;
      }
      if (strlen(record.fields[1]) >= 256) {
        fprintf(stderr, "Text font name too long: '%s'\n", record.fields[1]);
        continue;
      }
      if (!is_font_name_valid(record.fields[1])) {
        fprintf(stderr, "Text font name contains illegal characters: '%s'\n",
            record.fields[1]);
        continue;
      }
      strcpy(current_style.font_name, record.fields[1]);
      if (str_to_int(record.fields[2], &current_style.font_size)) {
        fprintf(stderr, "Text FONT command's 2nd option must be integer.\n");
        current_style.font_size = 12;
      }
      continue;
    }
    if (strcmp(record.fields[0], "OPTBREAK") == 0) {
      if (record.field_count != 4) {
        fprintf(stderr, "Text OPTBREAK command must have 3 options.\n");
        continue;
      }
      if (str_to_int(record.fields[3], &arg1)) {
        fprintf(stderr,
            "Text OPTBREAK command's 3rd option must be integer.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct break_gizmo)
          + strlen(record.fields[1])
          + strlen(record.fields[2]) + 2);
      (*next_gizmo)->type = GIZMO_BREAK;
      (*next_gizmo)->next = NULL;
      ((struct break_gizmo *)*next_gizmo)->spacing = arg1;
      ((struct break_gizmo *)*next_gizmo)->does_break = 0;
      ((struct break_gizmo *)*next_gizmo)->early_break = 0;
      ((struct break_gizmo *)*next_gizmo)->total_penalty = INT_MAX;
      ((struct break_gizmo *)*next_gizmo)->best_source = NULL;
      ((struct break_gizmo *)*next_gizmo)->style = current_style;
      ((struct break_gizmo *)*next_gizmo)->no_break_width
        = get_text_width(record.fields[1], &current_style, typeface);
      ((struct break_gizmo *)*next_gizmo)->at_break_width
        = get_text_width(record.fields[2], &current_style, typeface);
      ((struct break_gizmo *)*next_gizmo)->no_break
        = ((struct break_gizmo *)*next_gizmo)->strings;
      ((struct break_gizmo *)*next_gizmo)->at_break
        = ((struct break_gizmo *)*next_gizmo)->strings
        + strlen(record.fields[1]) + 1;
      strcpy(((struct break_gizmo *)*next_gizmo)->no_break,
          record.fields[1]);
      strcpy(((struct break_gizmo *)*next_gizmo)->at_break,
          record.fields[2]);
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
    if (strcmp(record.fields[0], "BREAK") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Text BREAK command must take 1 option.\n");
        continue;
      }
      if (str_to_int(record.fields[1], &arg1)) {
        fprintf(stderr, "Text BREAK command's 1st option must be integer.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct break_gizmo) + 1);
      (*next_gizmo)->type = GIZMO_BREAK;
      (*next_gizmo)->next = NULL;
      ((struct break_gizmo *)*next_gizmo)->spacing = arg1;
      ((struct break_gizmo *)*next_gizmo)->does_break = 1;
      ((struct break_gizmo *)*next_gizmo)->early_break = 1;
      ((struct break_gizmo *)*next_gizmo)->total_penalty = INT_MAX;
      ((struct break_gizmo *)*next_gizmo)->best_source = NULL;
      ((struct break_gizmo *)*next_gizmo)->style = current_style;
      ((struct break_gizmo *)*next_gizmo)->no_break_width = 0;
      ((struct break_gizmo *)*next_gizmo)->at_break_width = 0;
      ((struct break_gizmo *)*next_gizmo)->no_break
        = ((struct break_gizmo *)*next_gizmo)->at_break
        = ((struct break_gizmo *)*next_gizmo)->strings;
      ((struct break_gizmo *)*next_gizmo)->strings[0] = '\0';
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
    if (strcmp(record.fields[0], "MARK") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Text MARK command must have 1 option.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct mark_gizmo)
          + strlen(record.fields[1]) + 1);
      (*next_gizmo)->type = GIZMO_MARK;
      (*next_gizmo)->next = NULL;
      strcpy(((struct mark_gizmo *)*next_gizmo)->string, record.fields[1]);
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
  }
  *next_gizmo = malloc(sizeof(struct break_gizmo) + 1);
  (*next_gizmo)->type = GIZMO_BREAK;
  (*next_gizmo)->next = NULL;
  ((struct break_gizmo *)*next_gizmo)->spacing = 0;
  ((struct break_gizmo *)*next_gizmo)->does_break = 1;
  ((struct break_gizmo *)*next_gizmo)->early_break = 1;
  ((struct break_gizmo *)*next_gizmo)->total_penalty = INT_MAX;
  ((struct break_gizmo *)*next_gizmo)->best_source = NULL;
  ((struct break_gizmo *)*next_gizmo)->style = current_style;
  ((struct break_gizmo *)*next_gizmo)->no_break_width = 0;
  ((struct break_gizmo *)*next_gizmo)->at_break_width = 0;
  ((struct break_gizmo *)*next_gizmo)->no_break
    = ((struct break_gizmo *)*next_gizmo)->at_break
    = ((struct break_gizmo *)*next_gizmo)->strings;
  ((struct break_gizmo *)*next_gizmo)->strings[0] = '\0';
  free_record(&record);
  return first_gizmo;
}

static void
free_gizmos(struct gizmo *gizmo)
{
  struct gizmo *next;
  while (gizmo) {
    next = gizmo->next;
    free(gizmo);
    gizmo = next;
  }
}

static void
consider_breaks(struct gizmo *gizmo, int source_penalty,
    struct break_gizmo *source, int line_width)
{
  struct break_gizmo *break_gizmo;
  int width, penalty;
  width = 0;
  for (; gizmo; gizmo = gizmo->next) {
    switch (gizmo->type) {
    case GIZMO_TEXT:
      width += ((struct text_gizmo *)gizmo)->width;
      break;
    case GIZMO_BREAK:
      break_gizmo = (struct break_gizmo *)gizmo;
      if (width + break_gizmo->at_break_width <= line_width) {
        penalty = (line_width - width - break_gizmo->at_break_width) / 1000;
        if (break_gizmo->does_break)
          penalty = 0;
        if (break_gizmo->total_penalty > source_penalty + penalty) {
          break_gizmo->best_source = source;
          break_gizmo->total_penalty = source_penalty + penalty;
        }
      }
      width += break_gizmo->no_break_width;
      if (width > line_width)
        goto stop;
      if (break_gizmo->does_break)
        goto stop;
      break;
    }
  }
stop:
}

static void
optimise_breaks(struct gizmo *gizmo, int line_width)
{
  struct break_gizmo *last_break;
  last_break = NULL;
  consider_breaks(gizmo, 0, NULL, line_width);
  for (; gizmo; gizmo = gizmo->next)
    if (gizmo->type == GIZMO_BREAK) {
      last_break = (struct break_gizmo *)gizmo;
      consider_breaks(gizmo->next, last_break->total_penalty, last_break,
          line_width);
    }
  for (; last_break; last_break = last_break->best_source)
    last_break->does_break = 1;
}

static void
print_text(struct dbuffer *buffer, const struct style *old_style,
    const struct style *style, const char *string, int *spaces)
{
  if (strcmp(style->font_name, old_style->font_name)
      || style->font_size != old_style->font_size)
    dbuffer_printf(buffer, "FONT %s %d\n", style->font_name, style->font_size);
  if (*string == '\0')
    return;
  dbuffer_printf(buffer, "STRING \"");
  while (*string) {
    if (*string == '\n') {
      string++;
      continue;
    }
    if (*string == '"')
      dbuffer_putc(buffer, '\\');
    if (*string == ' ')
      (*spaces)++;
    dbuffer_putc(buffer, *string);
    string++;
  }
  dbuffer_printf(buffer, "\"\n");
}

static void
print_gizmos(FILE *output, struct gizmo *gizmo, int line_width, int align)
{
  int width, height, spaces;
  struct style style;
  struct dbuffer line;
  struct dbuffer line_marks;
  struct text_gizmo *text_gizmo;
  struct break_gizmo *break_gizmo;
  struct mark_gizmo *mark_gizmo;
  width = 0;
  height = 0;
  spaces = 0;
  style.font_name[0] = '\0';
  style.font_size = 0;
  dbuffer_init(&line, 1024 * 4, 1024 * 4);
  dbuffer_init(&line_marks, 1024, 1024);
  line_marks.data[0] = '\0';
  for (; gizmo; gizmo = gizmo->next) {
    switch (gizmo->type) {
    case GIZMO_TEXT:
      text_gizmo = (struct text_gizmo *)gizmo;
      if (text_gizmo->style.font_size > height)
        height = text_gizmo->style.font_size;
      width += text_gizmo->width;
      print_text(&line, &style, &text_gizmo->style, text_gizmo->string,
          &spaces);
      style = text_gizmo->style;
      break;
    case GIZMO_MARK:
      mark_gizmo = (struct mark_gizmo *)gizmo;
      dbuffer_printf(&line_marks, "^%s\n", mark_gizmo->string);
      break;
    case GIZMO_BREAK:
      break_gizmo = (struct break_gizmo *)gizmo;
      if (break_gizmo->style.font_size > height)
        height = break_gizmo->style.font_size;
      if (break_gizmo->does_break) {
        width += break_gizmo->at_break_width;
        print_text(&line, &style, &break_gizmo->style, break_gizmo->at_break,
            &spaces);
        if (line.size) {
          dbuffer_putc(&line, '\0');
          fprintf(output, "box %d\n", height);
          if (align == ALIGN_CENTRE) {
            fprintf(output, "START GRAPHIC\n");
            fprintf(output, "MOVE %d 0\n", (line_width - width) / 2000);
          } else if (align == ALIGN_RIGHT) {
            fprintf(output, "START GRAPHIC\n");
            fprintf(output, "MOVE %d 0\n", (line_width - width) / 1000);
          }
          fprintf(output, "START TEXT\n");
          if (align == ALIGN_JUSTIFIED && spaces && !break_gizmo->early_break) {
            fprintf(output, "SPACE %d\n", (line_width - width) / spaces);
          }
          fprintf(output, "%s", line.data);
          fprintf(output, "END\n");
          if (align == ALIGN_CENTRE || align == ALIGN_RIGHT) {
            fprintf(output, "END\n");
          }
        }
        fprintf(output, line_marks.data);
        line_marks.size = 0;
        line_marks.data[0] = '\0';
        if (break_gizmo->spacing)
          fprintf(output, "glue %d\n", break_gizmo->spacing);
        fprintf(output, "opt_break\n");
        height = 0;
        width = 0;
        spaces = 0;
        style.font_name[0] = '\0';
        style.font_size = 0;
        line.size = 0;
        line.data[0] = '\0';
      } else {
        width += break_gizmo->no_break_width;
        print_text(&line, &style, &break_gizmo->style, break_gizmo->no_break,
            &spaces);
        style = break_gizmo->style;
      }
      break;
    }
  }
  dbuffer_free(&line);
  dbuffer_free(&line_marks);
}

static void
die_usage(char *program_name)
{
  fprintf(stderr, "Usage: %s -l NUM\n", program_name);
  exit(1);
}

int
main(int argc, char **argv)
{
  int opt, line_width, align;
  FILE *input_file, *typeface_file, *output_file;
  struct typeface *typeface;
  struct gizmo *gizmos;
  line_width = 0;
  align = ALIGN_LEFT;
  while ( (opt = getopt(argc, argv, "ljrcw:")) != -1) {
    switch (opt) {
    case 'l':
      align = ALIGN_LEFT;
      break;
    case 'j':
      align = ALIGN_JUSTIFIED;
      break;
    case 'r':
      align = ALIGN_RIGHT;
      break;
    case 'c':
      align = ALIGN_CENTRE;
      break;
    case 'w':
      if (str_to_int(optarg, &line_width))
        die_usage(argv[0]);
      /* Convert point space to text space. */
      line_width *= 1000;
      break;
    default:
      die_usage(argv[0]);
    }
  }
  if (line_width == 0)
    die_usage(argv[0]);
  input_file = stdin;
  typeface_file = fopen("typeface", "r");
  output_file = stdout;
  typeface = open_typeface(typeface_file);
  fclose(typeface_file);
  gizmos = parse_gizmos(input_file, typeface);
  optimise_breaks(gizmos, line_width);
  print_gizmos(output_file, gizmos, line_width, align);
  free_typeface(typeface);
  free_gizmos(gizmos);
  fclose(input_file);
  fclose(output_file);
  return 0;
}
