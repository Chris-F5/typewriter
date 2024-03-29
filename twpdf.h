/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

enum pdf_obj_type {
  PDF_OBJ_BOOLEAN         = 0,
  PDF_OBJ_INTEGER         = 1,
  PDF_OBJ_STRING          = 2,
  PDF_OBJ_NAME            = 3,
  PDF_OBJ_ARRAY           = 4,
  PDF_OBJ_DICTIONARY      = 5,
  PDF_OBJ_STREAM          = 6,
  PDF_OBJ_NULL            = 7,
  PDF_OBJ_INDIRECT        = 8,
};

struct pdf_obj {
  enum pdf_obj_type type;
  char data[];
};

struct pdf_obj_boolean {
  enum pdf_obj_type type;
  int value;
};

struct pdf_obj_integer {
  enum pdf_obj_type type;
  int value;
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
  struct pdf_obj *value;
  struct pdf_obj_array *tail;
};

struct pdf_obj_dictionary {
  enum pdf_obj_type type;
  struct pdf_obj_name *key;
  struct pdf_obj *value;
  struct pdf_obj_dictionary *tail;
};

struct pdf_obj_stream {
  enum pdf_obj_type type;
  long size;
  char *bytes;
  struct pdf_obj_dictionary *dictionary;
};

struct pdf_obj_indirect {
  enum pdf_obj_type type;
  int obj_num;
};

struct pdf_indirect_obj_def {
  int obj_num;
  struct pdf_obj *obj;
  struct pdf_indirect_obj_def *next;
};

/* Stored in abstract format, only converted to pdf before writing to disk. */
struct pdf {
  int next_obj_num;
  struct pdf_indirect_obj_def *defs;
  struct pdf_indirect_obj_def *root;
  int obj_allocated, obj_count;
  struct pdf_obj **objs;
};

/* twpdf.c */
void pdf_init_empty(struct pdf *pdf);
void pdf_free(struct pdf *pdf);

struct pdf_obj_boolean         *pdf_create_boolean(struct pdf *pdf, int value);
struct pdf_obj_integer         *pdf_create_integer(struct pdf *pdf, int value);
struct pdf_obj_string          *pdf_create_string(struct pdf *pdf, const char *string);
struct pdf_obj_name            *pdf_create_name(struct pdf *pdf, const char *name);
struct pdf_obj_array           *pdf_create_array(struct pdf *pdf);
struct pdf_obj_dictionary      *pdf_create_dictionary(struct pdf *pdf);
struct pdf_obj_indirect        *pdf_allocate_indirect_obj(struct pdf *pdf);

struct pdf_obj_array *pdf_prepend_array(struct pdf *pdf,
    struct pdf_obj_array *array, struct pdf_obj *obj);
struct pdf_obj_dictionary *pdf_prepend_dictionary(struct pdf *pdf,
    struct pdf_obj_dictionary *dictionary, const char *key,
    struct pdf_obj *value);

void pdf_define_obj(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_obj *obj, int is_root);
void pdf_define_stream(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_obj_dictionary *dictionary, struct pdf_obj_array *filters,
    long size, char *bytes);

/* twwrite.c */
void pdf_write(struct pdf *pdf, const char *fname);
