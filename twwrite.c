/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "twpdf.h"

static void write_obj_boolean(FILE *file, const struct pdf_obj_boolean *obj);
static void write_obj_integer(FILE *file, const struct pdf_obj_integer *obj);
static void write_obj_string(FILE *file, const struct pdf_obj_string *obj);
static void write_obj_name(FILE *file, const struct pdf_obj_name *obj);
static void write_obj_array(FILE *file, const struct pdf_obj_array *obj);
static void write_obj_dictionary(FILE *file, const struct pdf_obj_dictionary *obj);
static void write_obj_stream(FILE *file, const struct pdf_obj_stream *obj);
static void write_obj_null(FILE *file);
static void write_obj_indirect(FILE *file, const struct pdf_obj_indirect *obj);
static void write_obj(FILE *file, const struct pdf_obj *obj);

static void
write_obj_boolean(FILE *file, const struct pdf_obj_boolean *obj)
{
  fprintf(file, obj ? "true" : "false");
}

static void
write_obj_integer(FILE *file, const struct pdf_obj_integer *obj)
{
  fprintf(file, "%d", obj->value);
}

static void
write_obj_string(FILE *file, const struct pdf_obj_string *obj)
{
  const unsigned char *c;
  fputc('(', file);
  for (c = (const unsigned char *)obj->string; *c; c++) {
    if (*c > 127) {
      fprintf(stderr, "twpdf: Non-ASCII characters are not supported.\n");
      exit(1);
    }
    switch (*c) {
    case '(':
    case ')':
    case '\\':
      fputc('\\', file);
    default:
      fputc(*c, file);
    }
  }
  fputc(')', file);
}

static void
write_obj_name(FILE *file, const struct pdf_obj_name *obj)
{
  const unsigned char *c;
  fputc('/', file);
  for (c = (const unsigned char *)obj->string; *c; c++) {
    if (*c > 127) {
      fprintf(stderr, "twpdf: Non-ASCII characters are not supported.\n");
      exit(1);
    }
    switch (*c) {
    case '\t':
    case '\n':
    case '\f':
    case '\r':
    case ' ':
    case '(':
    case ')':
    case '<':
    case '>':
    case '[':
    case ']':
    case '{':
    case '}':
    case '/':
    case '%':
    case '#':
      fprintf(file, "#%02x", *c);
      break;
    default:
      fputc(*c, file);
    }
  }
}

static void
write_obj_array(FILE *file, const struct pdf_obj_array *obj)
{
  fputc('[', file);
  for (; obj->value; obj = obj->tail) {
    write_obj(file, obj->value);
    if (obj->tail->value)
      fputc(' ', file);
  }
  fputc(']', file);
}

static void
write_obj_dictionary(FILE *file, const struct pdf_obj_dictionary *obj)
{
  fprintf(file, "<< ");
  for (; obj->key; obj = obj->tail) {
    write_obj_name(file, obj->key);
    fputc(' ', file);
    write_obj(file, obj->value);
    fputc(' ', file);
  }
  fprintf(file, ">>");
}

static void
write_obj_stream(FILE *file, const struct pdf_obj_stream *obj)
{
  long i;
  write_obj_dictionary(file, obj->dictionary);
  fprintf(file, "\nstream\n");
  for (i = 0; i < obj->size; i++)
    fprintf(file, "%02x", (unsigned char)obj->bytes[i]);
  fprintf(file, "\nendstream");
}

static void
write_obj_null(FILE *file)
{
  fprintf(file, "null");
}

static void
write_obj_indirect(FILE *file, const struct pdf_obj_indirect *obj)
{
  fprintf(file, "%d 0 R", obj->obj_num);
}


static void
write_obj(FILE *file, const struct pdf_obj *obj)
{
  switch (obj->type) {
  case PDF_OBJ_BOOLEAN:
    write_obj_boolean(file, (struct pdf_obj_boolean *)obj);
    break;
  case PDF_OBJ_INTEGER:
    write_obj_integer(file, (struct pdf_obj_integer *)obj);
    break;
  case PDF_OBJ_STRING:
    write_obj_string(file, (struct pdf_obj_string *)obj);
    break;
  case PDF_OBJ_NAME:
    write_obj_name(file, (struct pdf_obj_name *)obj);
    break;
  case PDF_OBJ_ARRAY:
    write_obj_array(file, (struct pdf_obj_array *)obj);
    break;
  case PDF_OBJ_DICTIONARY:
    write_obj_dictionary(file, (struct pdf_obj_dictionary *)obj);
    break;
  case PDF_OBJ_STREAM:
    write_obj_stream(file, (struct pdf_obj_stream *)obj);
    break;
  case PDF_OBJ_NULL:
    write_obj_null(file);
    break;
  case PDF_OBJ_INDIRECT:
    write_obj_indirect(file, (struct pdf_obj_indirect *)obj);
    break;
  default:
    fprintf(stderr, "twpdf: Unknown object type %d.\n", obj->type);
    exit(1);
  }
}

void
pdf_write(struct pdf *pdf, const char *fname)
{
  FILE *file;
  struct pdf_indirect_obj_def *def;
  long *xref_obj_offsets;
  long xref_offset;
  int i;
  if (pdf->root == NULL) {
    fprintf(stderr, "twpdf: Cant write pdf without a root.\n");
    exit(1);
  }
  file = fopen(fname, "w");
  if (file == NULL) {
    fprintf(stderr, "twpdf: Failed to open file %s.\n", fname);
    exit(1);
  }
  /* Header */
  fprintf(file, "%%PDF-1.7\n");
  /* Body */
  xref_obj_offsets = xmalloc(pdf->next_obj_num * sizeof(long));
  memset(xref_obj_offsets, 0, pdf->next_obj_num * sizeof(long));
  for (def = pdf->defs; def; def = def->next) {
    if (def->obj_num > pdf->next_obj_num) {
      fprintf(stderr, "twpdf: Unexpected object number in definition.\n");
      exit(1);
    }
    xref_obj_offsets[def->obj_num] = ftell(file);
    fprintf(file, "%d 0 obj\n", def->obj_num);
    write_obj(file, def->obj);
    fprintf(file, "\nendobj\n");
  }
  /* Cross-Reference Table */
  xref_offset = ftell(file);
  fprintf(file, "xref\n");
  fprintf(file, "0 %d\n", pdf->next_obj_num);
  fprintf(file, "0000000000 65535 f \n");
  for (i = 1; i < pdf->next_obj_num; i++)
    fprintf(file, "%010ld 00000 %c \n", xref_obj_offsets[i],
        xref_obj_offsets[i] ? 'n' : 'f');
  /* Trailer */
  fprintf(file, "trailer << /Size %d /Root %d 0 R >>\n", pdf->next_obj_num,
      pdf->root->obj_num);
  fprintf(file, "startxref\n");
  fprintf(file, "%ld\n", xref_offset);
  fprintf(file, "%%%%EOF");

  /* TODO: Check for file write errors. */
  
  free(xref_obj_offsets);
  fclose(file);
}
