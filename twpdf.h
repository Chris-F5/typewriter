/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * 8 Basic Objects (7.3.1):
 * Boolean, Integer/Real, String, Name, Array, Dictionary, Stream, Null
 *
 * File Structure (7.5):
 *   Header, Body, Cross-reference table, Trailer
 * Generally, lines are limited to 255 chars.
 */

enum pdf_obj_type {
  PDF_PRIMITIVE_BOOLEAN         = 0,
  PDF_PRIMITIVE_NUMERIC         = 1,
  PDF_PRIMITIVE_STRING          = 2,
  PDF_PRIMITIVE_NAME            = 3,
  PDF_PRIMITIVE_ARRAY           = 4,
  PDF_PRIMITIVE_DICTIONARY      = 5,
  PDF_PRIMITIVE_GRAPHICS_STREAM = 6,
  PDF_PRIMITIVE_NULL            = 7,
  PDF_PRIMITIVE_INDIRECT        = 8,
};

struct pdf_obj {
  enum pdf_obj_type type;
  char data[];
};

struct pdf_obj_boolean {
  enum pdf_obj_type type;
  int value;
};

struct pdf_obj_numeric {
  enum pdf_obj_type type;
  double vlaue;
};

struct pdf_obj_string {
  enum pdf_obj_type type;
  const char *string;
};

struct pdf_obj_name {
  enum pdf_obj_type type;
  const char *string;
};

struct pdf_obj_array {
  enum pdf_obj_type type;
  struct pdf_obj *head;
  struct pdf_obj_array *tail;
};

struct pdf_obj_dictionary {
  enum pdf_obj_type type;
  struct pdf_obj *key, *value;
  struct pdf_obj_dictionary *next;
};

/* > All streams shall be indirect objects (7.3.8.1). */
struct pdf_obj_graphics_stream {
  enum pdf_obj_type type;
  /* ... */
};

struct pdf_obj_indirect {
  enum pdf_obj_type type;
  int obj_num, gen_num;
};

struct pdf_indirect_obj_def {
  int obj_num, gen_num;
  struct pdf_obj *obj;
  struct pdf_indirect_obj_def *next;
};

struct pdf {
  /* Stored in abstract format, only converted to pdf before writing to disk. */
  int next_obj_num;
  struct pdf_indirect_obj_def *defs;
  int obj_allocated, obj_count;
  struct pdf_obj *objs;
  struct pdf_obj *null_obj;
};

/* twpdf.c */
void pdf_init_empty(struct pdf *pdf);
void pdf_free(struct pdf *pdf);

struct pdf_obj_boolean         *pdf_create_boolean(struct pdf *pdf, int value);
struct pdf_obj_numeric         *pdf_create_numeric(struct pdf *pdf, double value);
struct pdf_obj_string          *pdf_create_string(struct pdf *pdf, const char *string);
struct pdf_obj_name            *pdf_create_name(struct pdf *pdf, const char *name);
struct pdf_obj_array           *pdf_create_array(struct pdf *pdf);
struct pdf_obj_dictionary      *pdf_create_dictionary(struct pdf *pdf);
struct pdf_obj_graphics_stream *pdf_create_graphics_stream(struct pdf *pdf);
struct pdf_obj_null            *pdf_create_null(struct pdf *pdf);
struct pdf_obj_indirect        *pdf_allocate_indirect_obj(struct pdf *pdf);

struct pdf_obj_array *pdf_prepend_array(struct pdf *pdf,
    struct pdf_obj_array *array, struct pdf_obj *obj);
struct pdf_obj_array *pdf_prepend_dictionary(struct pdf *pdf,
    struct pdf_obj_dictionary *dictionary, struct pdf_obj_name *key,
    struct pdf_obj *value);

void pdf_define_obj(struct pdf *pdf, struct pdf_obj *obj,
    struct pdf_obj_indirect *ref);

void pdf_write(struct pdf *pdf, const char *fname);
