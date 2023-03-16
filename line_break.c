/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tw.h"

enum gizmo_type {
  GIZMO_STRING,
  GIZMO_FONT,
  GIZMO_OPTBREAK,
  GIZMO_BREAK,
};

struct gizmo {
  int type;
  struct gizmo *next;
  char _[];
};

struct string_gizmo {
  int type; /* GIZMO_STRING */
  struct gizmo *next;
  char string[];
};

struct font_gizmo {
  int type; /* GIZMO_FONT */
  struct gizmo *next;
  int font_size;
  char font_name[];
};

struct optbreak_gizmo {
  int type; /* GIZMO_OPTBREAK */
  struct gizmo *next;
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
  int parse_result;
  struct record record;
  first_gizmo = NULL;
  next_gizmo = &first_gizmo;
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
      *next_gizmo = malloc(sizeof(struct string_gizmo)
          + strlen(record.fields[1]) + 1);
      (*next_gizmo)->type = GIZMO_STRING;
      (*next_gizmo)->next = NULL;
      strcpy(((struct string_gizmo *)*next_gizmo)->string, record.fields[1]);
      next_gizmo = &(*next_gizmo)->next;
      continue;
    }
    if (strcmp(record.fields[0], "FONT") == 0) {
      if (record.field_count != 3) {
        fprintf(stderr, "Text FONT command must have 2 options.\n");
        continue;
      }
      *next_gizmo = malloc(sizeof(struct font_gizmo) + strlen(record.fields[1])
          + 1);
      (*next_gizmo)->type = GIZMO_FONT;
      (*next_gizmo)->next = NULL;
      strcpy(((struct font_gizmo *)*next_gizmo)->font_name, record.fields[1]);
      if (str_to_int(record.fields[2],
            &((struct font_gizmo *)*next_gizmo)->font_size)) {
        fprintf(stderr, "Text FONT command's 2nd option must be integer.\n");
        ((struct font_gizmo *)*next_gizmo)->font_size = 12;
      }
      next_gizmo = &(*next_gizmo)->next;
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
