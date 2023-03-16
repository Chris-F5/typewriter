/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tw.h"

enum gizmo_type {
  GIZMO_TEXT,
  GIZMO_OPTBREAK,
  GIZMO_BREAK,
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
  struct style style;
  char string[];
};

struct optbreak_gizmo {
  int type; /* GIZMO_OPTBREAK */
  struct gizmo *next;
  struct style style;
  char *no_break, *at_break;
  char strings[];
};

static struct gizmo *parse_gizmos(FILE *file);
static void free_gizmos(struct gizmo *gizmo);

static struct gizmo *
parse_gizmos(FILE *file)
{
  struct gizmo *first_gizmo;
  struct gizmo **next_gizmo;
  struct style current_style;
  int parse_result;
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
      if (record.field_count != 3) {
        fprintf(stderr, "Text OPTBREAK command must have 2 options.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct optbreak_gizmo)
          + strlen(record.fields[1])
          + strlen(record.fields[2]) + 2);
      (*next_gizmo)->type = GIZMO_OPTBREAK;
      (*next_gizmo)->next = NULL;
      ((struct optbreak_gizmo *)*next_gizmo)->style = current_style;
      ((struct optbreak_gizmo *)*next_gizmo)->no_break
        = ((struct optbreak_gizmo *)*next_gizmo)->strings;
      ((struct optbreak_gizmo *)*next_gizmo)->at_break
        = ((struct optbreak_gizmo *)*next_gizmo)->strings
        + strlen(record.fields[1]) + 1;
      strcpy(((struct optbreak_gizmo *)*next_gizmo)->no_break,
          record.fields[1]);
      strcpy(((struct optbreak_gizmo *)*next_gizmo)->at_break,
          record.fields[2]);
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
    if (strcmp(record.fields[0], "BREAK") == 0) {
      if (record.field_count != 1) {
        fprintf(stderr, "Text BREAK command takes no options.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct gizmo));
      (*next_gizmo)->type = GIZMO_BREAK;
      (*next_gizmo)->next = NULL;
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
  }
  free_record(&record);
  return first_gizmo;
}

static void
free_gizmos(struct gizmo *gizmo)
{
  struct gizmo *next_gizmo;
  while (gizmo) {
    next_gizmo = gizmo->next;
    free(gizmo);
    gizmo = next_gizmo;
  }
}

int
main(int argc, char **argv)
{
  FILE *input_file;
  struct gizmo* gizmos;
  input_file = stdin;
  gizmos = parse_gizmos(input_file);
  free_gizmos(gizmos);
  return 0;
}
