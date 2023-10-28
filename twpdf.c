/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "twpdf.h"
#include "utils.h"

static void * allocate_obj(struct pdf *pdf, size_t size);
static void free_obj(struct pdf_obj *obj);

static void *
allocate_obj(struct pdf *pdf, size_t size)
{
  if (pdf->obj_count == pdf->obj_allocated) {
    pdf->obj_allocated += 50;
    pdf->objs = xrealloc(pdf->objs, pdf->obj_allocated * sizeof(void *));
  }
  return pdf->objs[pdf->obj_count++] = xmalloc(size);
}

static void
free_obj(struct pdf_obj *obj)
{
  switch (obj->type) {
  case PDF_OBJ_STREAM:
    free(((struct pdf_obj_stream *)obj)->bytes);
    break;
  default:
    break;
  }
  free(obj);
}

void
pdf_init_empty(struct pdf *pdf)
{
  pdf->next_obj_num = 1;
  pdf->defs = NULL;
  pdf->root = NULL;
  pdf->obj_allocated = 1000;
  pdf->obj_count = 0;
  pdf->objs = xmalloc(pdf->obj_allocated * sizeof(void *));
  pdf->objs[0] = NULL;
}

void
pdf_free(struct pdf *pdf)
{
  struct pdf_indirect_obj_def *obj_def, *next_obj_def;
  int i;
  obj_def = pdf->defs;
  while (obj_def) {
    next_obj_def = obj_def->next;
    free(obj_def);
    obj_def = next_obj_def;
  }
  for (i = 0; i < pdf->obj_count; i++)
    free_obj(pdf->objs[i]);
  free(pdf->objs);
}

struct pdf_obj_boolean *
pdf_create_boolean(struct pdf *pdf, int value)
{
  struct pdf_obj_boolean *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_boolean));
  obj->type = PDF_OBJ_BOOLEAN;
  obj->value = value ? 1 : 0;
  return obj;
}

struct pdf_obj_integer *
pdf_create_integer(struct pdf *pdf, int value)
{
  struct pdf_obj_integer *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_integer));
  obj->type = PDF_OBJ_INTEGER;
  obj->value = value;
  return obj;
}

struct pdf_obj_string *
pdf_create_string(struct pdf *pdf, const char *string)
{
  struct pdf_obj_string *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_string));
  obj->type = PDF_OBJ_STRING;
  obj->string = string;
  return obj;
}

struct pdf_obj_name *
pdf_create_name(struct pdf *pdf, const char *name)
{
  struct pdf_obj_name *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_name));
  obj->type = PDF_OBJ_NAME;
  obj->string = name;
  return obj;
}

struct pdf_obj_array *
pdf_create_array(struct pdf *pdf)
{
  struct pdf_obj_array *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_array));
  obj->type = PDF_OBJ_ARRAY;
  obj->value = NULL;
  obj->tail = NULL;
  return obj;
}

struct pdf_obj_dictionary *
pdf_create_dictionary(struct pdf *pdf)
{
  struct pdf_obj_dictionary *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_dictionary));
  obj->type = PDF_OBJ_DICTIONARY;
  obj->key = NULL;
  obj->value = NULL;
  obj->tail = NULL;
  return obj;
}

struct pdf_obj_indirect *
pdf_allocate_indirect_obj(struct pdf *pdf)
{
  struct pdf_obj_indirect *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_indirect));
  obj->type = PDF_OBJ_INDIRECT;
  obj->obj_num = pdf->next_obj_num++;
  return obj;
}

struct pdf_obj_array *
pdf_prepend_array(struct pdf *pdf, struct pdf_obj_array *array,
    struct pdf_obj *obj)
{
  struct pdf_obj_array *head;
  head = allocate_obj(pdf, sizeof(struct pdf_obj_array));
  head->type = PDF_OBJ_ARRAY;
  head->value = obj;
  head->tail = array;
  return head;
}

struct pdf_obj_dictionary *
pdf_prepend_dictionary(struct pdf *pdf, struct pdf_obj_dictionary *dictionary,
    const char *key, struct pdf_obj *value)
{
  struct pdf_obj_dictionary *head;
  head = allocate_obj(pdf, sizeof(struct pdf_obj_dictionary));
  head->type = PDF_OBJ_DICTIONARY;
  head->key = pdf_create_name(pdf, key);
  head->value = value;
  head->tail = dictionary;
  return head;
}

void
pdf_define_obj(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_obj *obj, int is_root)
{
  struct pdf_indirect_obj_def *def;
  def = xmalloc(sizeof(struct pdf_indirect_obj_def));
  def->obj_num = ref->obj_num;
  def->obj = obj;
  def->next = pdf->defs;
  pdf->defs = def;
  if (is_root && pdf->root) {
    fprintf(stderr, "twpdf: pdf cant have two root objects.\n");
    exit(1);
  } else if (is_root) {
    pdf->root = def;
  }
}

void
pdf_define_stream(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_obj_dictionary *dictionary, struct pdf_obj_array *filters,
    long size, char *bytes)
{
  struct pdf_obj_stream *stream;
  stream = allocate_obj(pdf, sizeof(struct pdf_obj_stream));
  stream->type = PDF_OBJ_STREAM;
  stream->size = size;
  stream->bytes = bytes;
  filters = pdf_prepend_array(pdf, filters,
      (struct pdf_obj *)pdf_create_name(pdf, "ASCIIHexDecode"));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Filter",
      (struct pdf_obj *)filters);
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Length",
      (struct pdf_obj *)pdf_create_integer(pdf, size * 2));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Length1",
      (struct pdf_obj *)pdf_create_integer(pdf, size));
  stream->dictionary = dictionary;
  pdf_define_obj(pdf, ref, (struct pdf_obj *)stream, 0);
}
