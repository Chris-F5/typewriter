/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * The following must be included before this file:
#include "twpdf.h"
 */

enum gizmo_type {
  GIZMO_TEXT,
  GIZMO_IMAGE,
  GIZMO_GLUE,
};

struct gizmo {
  int type;
  struct gizmo *next;
  char data[];
};

struct gizmo_text {
  int type;
  struct gizmo *next;
  int font_size;
  const char *str;
};

struct gizmo_image {
  int type;
  struct gizmo *next;
  int w, h;
  const char *name;
};

struct gizmo_glue {
  int type;
  struct gizmo *next;
  int break_penalty, no_break_height;
  /* Shortest path attributes. */
  struct gizmo_glue *best_source;
  int best_total_penalty;
  int is_optimal;
};

struct document {
  int top_margin, bot_margin, left_margin;
  struct pdf pdf;
  struct pdf_obj_dictionary *xobjects;
  struct gizmo *gizmos;
  struct gizmo **gizmos_end;
};

int gizmo_height(const struct gizmo *gizmo);
void optimise_breaks(struct document *doc);
void init_document(struct document *doc, int top_margin, int bot_margin, int left_margin);
void free_document(struct document *doc);
void build_document(struct document *doc);
void put_text(struct document *doc, const char *str, int font_size);
void put_image(struct document *doc, const char *fname, int w);
void put_glue(struct document *doc, int break_penalty, int no_break_height);
