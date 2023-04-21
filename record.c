/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tw.h"

void
init_record(struct record *record)
{
  dbuffer_init(&record->string, 1024 * 2, 1024 * 2);
  record->field_count = 0;
  record->fields_allocated = 32;
  record->fields = xmalloc(record->fields_allocated * sizeof(char *));
}

void
begin_field(struct record *record)
{
  if (record->field_count == record->fields_allocated) {
    record->fields_allocated += 32;
    record->fields = xrealloc(record->fields, record->fields_allocated);
  }
  record->fields[record->field_count++] = record->string.data
    + record->string.size;
}

enum ParseState {
  PARSE_BEGIN,
  PARSE_NORMAL_FIELD,
  PARSE_NORMAL_FIELD_ESCAPE,
  PARSE_QUOTED_FIELD,
  PARSE_QUOTED_FIELD_ESCAPE,
  PARSE_OUTSIDE_FIELD,
  /* TERMINATING STATES */
  PARSE_FINISH_RECORD,
  PARSE_EOF,
  PARSE_UNTERMINATED_STRING,
  PARSE_UNTERMINATED_ESCAPE,
};

int
parse_record(FILE *file, struct record *record)
{
  int c, state;
  state = PARSE_BEGIN;
  record->string.size = 0;
  record->field_count = 0;
  while (state < PARSE_FINISH_RECORD) {
    c = fgetc(file);
    switch (state) {
    case PARSE_BEGIN:
      if (c == EOF) {
        state = PARSE_EOF;
      } else if (c == ' ' || c == '\n') {
        continue;
      } else if (c == '"') {
        state = PARSE_QUOTED_FIELD;
        begin_field(record);
      } else if (c == '\\') {
        state = PARSE_NORMAL_FIELD_ESCAPE;
        begin_field(record);
      } else {
        state = PARSE_NORMAL_FIELD;
        begin_field(record);
        dbuffer_putc(&record->string, (char)c);
      }
      break;
    case PARSE_NORMAL_FIELD:
      if (c == EOF) {
        state = PARSE_EOF;
        dbuffer_putc(&record->string, '\0');
      } else if (c == '\\') {
        state = PARSE_NORMAL_FIELD_ESCAPE;
      } else if (c == ' ') {
        state = PARSE_OUTSIDE_FIELD;
        dbuffer_putc(&record->string, '\0');
      } else if (c == '\n') {
        state = PARSE_FINISH_RECORD;
        dbuffer_putc(&record->string, '\0');
      } else {
        dbuffer_putc(&record->string, (char)c);
      }
      break;
    case PARSE_NORMAL_FIELD_ESCAPE:
      if (c == EOF) {
        state = PARSE_UNTERMINATED_ESCAPE;
      } else {
        state = PARSE_NORMAL_FIELD;
        dbuffer_putc(&record->string, (char)c);
      }
      break;
    case PARSE_QUOTED_FIELD:
      if (c == EOF) {
        state = PARSE_UNTERMINATED_STRING;
      } else if (c == '\\') {
        state = PARSE_QUOTED_FIELD_ESCAPE;
      } else if (c == '"') {
        state = PARSE_OUTSIDE_FIELD;
        dbuffer_putc(&record->string, '\0');
      } else {
        dbuffer_putc(&record->string, (char)c);
      }
      break;
    case PARSE_QUOTED_FIELD_ESCAPE:
      if (c == EOF) {
        state = PARSE_UNTERMINATED_ESCAPE;
      } else {
        state = PARSE_QUOTED_FIELD;
        dbuffer_putc(&record->string, (char)c);
      }
      break;
    case PARSE_OUTSIDE_FIELD:
      if (c == EOF) {
        state = PARSE_EOF;
      } else if (c == '\n') {
        state = PARSE_FINISH_RECORD;
      } else if (c == ' ') {
        continue;
      } else if (c == '"') {
        state = PARSE_QUOTED_FIELD;
        begin_field(record);
      } else if (c == '\\') {
        state = PARSE_NORMAL_FIELD_ESCAPE;
        begin_field(record);
      } else {
        state = PARSE_NORMAL_FIELD;
        begin_field(record);
        dbuffer_putc(&record->string, (char)c);
      }
      break;
    default:
      fprintf(stderr, "Unexpected state while parsing record.\n");
      break;
    }
  }
  switch (state) {
  case PARSE_UNTERMINATED_STRING:
    fprintf(stderr, "Failed to parse record: unterminated string.\n");
    goto error;
  case PARSE_UNTERMINATED_ESCAPE:
    fprintf(stderr, "Failed to parse record: unterminated escape sequence.\n");
    goto error;
  case PARSE_EOF:
    return EOF;
  case PARSE_FINISH_RECORD:
    return 0;
  default:
    fprintf(stderr, "Unexpected terminating state while parsing record.\n");
    goto error;
  }
error:
  record->field_count = 0;
  record->string.size = 0;
  return 1;
}

int
find_field(const struct record *record, const char *field_str)
{
  int i;
  for (i = 0; i < record->field_count; i++)
    if (strcmp(record->fields[i], field_str) == 0)
      return i;
  return -1;
}

void
free_record(struct record *record)
{
  dbuffer_free(&record->string);
  free(record->fields);
}
