#include <stdio.h>
#include <stdlib.h>

#include "tw.h"

void
init_record(struct record *record)
{
  dbuffer_init(&record->string, 1024 * 2, 1024 * 2);
  record->field_count = 0;
  record->fields_allocated = 12;
  record->fields = xmalloc(record->fields_allocated * sizeof(char *));
}

int
parse_record(FILE *file, struct record *record)
{
  int c, in_field, in_string, escape_next;
  in_field = 0;
  in_string = 0;
  escape_next = 0;
  record->string.size = 0;
  record->field_count = 0;
  for (;;) {
    c = fgetc(file);
    if (c == EOF || (c == '\n' && record->field_count)) {
      if (in_field)
        dbuffer_putc(&record->string, '\0');
      break;
    }
    if (!in_field && c != ' ' && c != '\n') {
      /* Start field. */
      in_field = 1;
      if (record->field_count == record->fields_allocated) {
        record->fields_allocated += 12;
        record->fields = xrealloc(record->fields, record->fields_allocated);
      }
      record->fields[record->field_count++]
        = record->string.data + record->string.size;
      if (c == '"')
        in_string = 1;
      else
        dbuffer_putc(&record->string, (char)c);
      continue;
    }
    if ((in_field && in_string && escape_next == 0 && c == '"')
        || (in_field && !in_string && escape_next == 0 && c == ' ')) {
      /* End field. */
      in_field = 0;
      in_string = 0;
      dbuffer_putc(&record->string, '\0');
      continue;
    }
    if (in_field && escape_next == 0 && c == '\\') {
      /* Escape next character. */
      escape_next = 1;
      continue;
    }
    if (in_field) {
      /* Add character to field. */
      dbuffer_putc(&record->string, c);
      escape_next = 0;
      continue;
    }
  }
  if (escape_next) {
    fprintf(stderr, "Failed to parse record: unterminated character escape.\n");
    goto parse_error;
  }
  if (in_string) {
    fprintf(stderr, "Failed to parse record: unterminated string.\n");
    goto parse_error;
  }
  if (record->field_count == 0)
    return EOF;
  return 0;
parse_error:
  record->field_count = 0;
  record->string.size = 0;
  return 1;
}

void
free_record(struct record *record)
{
  dbuffer_free(&record->string);
  free(record->fields);
}
