/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "twpdf.h"
#include "utils.h"

static void *
allocate_obj(struct pdf *pdf, size_t size)
{
  if (pdf->obj_count == pdf->obj_allocated) {
    pdf->obj_allocated += 50;
    pdf->objs = xrealloc(pdf->objs, pdf->obj_allocated * sizeof(void *));
  }
  return pdf->objs[pdf->obj_count++] = xmalloc(size);
}

void
pdf_init_empty(struct pdf *pdf)
{
  pdf->next_obj_num = 0;
  pdf->defs = NULL;
  pdf->root = NULL;
  pdf->obj_allocated = 1000;
  pdf->obj_count = 0;
  pdf->objs = xmalloc(pdf->obj_allocated * sizeof(void *));
}

void
pdf_free(struct pdf *pdf)
{
  struct pdf_indirect_obj_def *obj_def, *next_obj_def;
  obj_def = pdf->defs;
  while (obj_def) {
    next_obj_def = obj_def->next;
    free(obj_def);
    obj_def = next_obj_def;
  }
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

struct pdf_obj_numeric *
pdf_create_numeric(struct pdf *pdf, double value)
{
  struct pdf_obj_numeric *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_numeric));
  obj->type = PDF_OBJ_NUMERIC;
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

struct pdf_obj_graphics_stream *
pdf_create_graphics_stream(struct pdf *pdf)
{
  struct pdf_obj_graphics_stream *obj;
  obj = allocate_obj(pdf, sizeof(struct pdf_obj_graphics_stream));
  obj->type = PDF_OBJ_GRAPHICS_STREAM;
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
    struct pdf_obj_name *key, struct pdf_obj *value)
{
  struct pdf_obj_dictionary *head;
  head = allocate_obj(pdf, sizeof(struct pdf_obj_dictionary));
  head->type = PDF_OBJ_DICTIONARY;
  head->key = key;
  head->value = value;
  head->tail = dictionary;
  return head;
}

void
pdf_define_obj(struct pdf *pdf, struct pdf_obj *obj,
    struct pdf_obj_indirect *ref, int is_root)
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
