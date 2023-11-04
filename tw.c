/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "twpdf.h"
#include "twpages.h"
#include "twcontent.h"
#include "twjpeg.h"
#include "stralloc.h"
#include "utils.h"

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
  char *name;
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
  struct pdf pdf;
  struct pdf_obj_dictionary *xobjects;

  struct gizmo *gizmos;
  struct gizmo **gizmos_end;
};

static int gizmo_height(const struct gizmo *gizmo);
static void relax_glue(struct gizmo_glue *start, struct gizmo_glue *sentinel);
static void optimise_breaks(struct document *doc);
static void init_document(struct document *doc);
static void free_document(struct document *doc);
static void build_document(struct document *doc);
static void put_text(struct document *doc, const char *str);
static void put_glue(struct document *doc, int break_penalty, int no_break_height);
static int read_line(FILE *file, char **line, int *allocated);
static void read_file(struct document *doc, FILE *file);

static const int font_size = 9;
static const int top_margin = 40;
static const int bot_margin = 40;
static const int left_margin = 80;

struct stralloc stralloc;

static int
gizmo_height(const struct gizmo *gizmo)
{
  const struct gizmo_text *text;
  const struct gizmo_image *image;
  const struct gizmo_glue *glue;
  switch (gizmo->type) {
  case GIZMO_TEXT:
    text = (struct gizmo_text *)gizmo;
    return text->font_size;
  case GIZMO_IMAGE:
    image = (struct gizmo_image *)gizmo;
    return image->h;
  case GIZMO_GLUE:
    glue = (struct gizmo_glue *)gizmo;
    return glue->no_break_height;
  }
  fprintf(stderr, "tw: Unknown gizmo type %d.\n", gizmo->type);
  exit(1);
}

static void
relax_glue(struct gizmo_glue *start, struct gizmo_glue *sentinel)
{
  int used_height, total_penalty, stop;
  used_height = 0;
  stop = 0;
  struct gizmo *gizmo;
  struct gizmo_glue *glue;
  for (gizmo = start->next; !stop && gizmo != (struct gizmo *)sentinel; gizmo = gizmo->next) {
    if (gizmo->type == GIZMO_GLUE) {
      glue = (struct gizmo_glue *)gizmo;
      total_penalty = start->best_total_penalty + start->break_penalty;
      if (used_height > 842 - top_margin - bot_margin) {
        total_penalty += 10000;
        stop = 1;
      } else {
        total_penalty += 842 - used_height - top_margin - bot_margin;
      }
      if (glue->best_source == NULL || glue->best_total_penalty > total_penalty) {
        glue->best_source = start;
        glue->best_total_penalty = total_penalty;
      }
    }
    used_height += gizmo_height(gizmo);
  }
  if (gizmo == (struct gizmo *)sentinel) {
    total_penalty = start->best_total_penalty + start->break_penalty;
    if (used_height > 842 - top_margin - bot_margin)
      total_penalty += 10000;
    if (sentinel->best_source == NULL || sentinel->best_total_penalty > total_penalty) {
      sentinel->best_source = start;
      sentinel->best_total_penalty = total_penalty;
    }
  }
}

static void
optimise_breaks(struct document *doc)
{
  struct gizmo *gizmo;
  struct gizmo_glue start_gizmo, *sentinel_gizmo, *glue;
  sentinel_gizmo = xmalloc(sizeof(struct gizmo_glue));
  sentinel_gizmo->type = GIZMO_GLUE;
  sentinel_gizmo->next = NULL;
  sentinel_gizmo->break_penalty = 0;
  sentinel_gizmo->no_break_height = 0;
  sentinel_gizmo->best_source = NULL;
  sentinel_gizmo->best_total_penalty = 0;
  sentinel_gizmo->is_optimal = 0;
  *doc->gizmos_end = (struct gizmo *)sentinel_gizmo;
  start_gizmo.type = GIZMO_GLUE;
  start_gizmo.next = doc->gizmos;
  start_gizmo.break_penalty = 0;
  start_gizmo.no_break_height = 0;
  start_gizmo.best_source = NULL;
  start_gizmo.best_total_penalty = 0;
  start_gizmo.is_optimal = 0;
  relax_glue(&start_gizmo, sentinel_gizmo);
  for (gizmo = doc->gizmos; gizmo != (struct gizmo *)sentinel_gizmo; gizmo = gizmo->next)
    if (gizmo->type == GIZMO_GLUE)
      relax_glue((struct gizmo_glue *)gizmo, sentinel_gizmo);
  for (glue = sentinel_gizmo; glue != &start_gizmo; glue = glue->best_source)
    glue->is_optimal = 1;
  *doc->gizmos_end = NULL;
  free(sentinel_gizmo);
}

static void
init_document(struct document *doc)
{
  pdf_init_empty(&doc->pdf);
  doc->xobjects = pdf_create_dictionary(&doc->pdf);
  doc->gizmos = NULL;
  doc->gizmos_end = &doc->gizmos;
}

static void
free_document(struct document *doc)
{
  struct gizmo *gizmo, *next_gizmo;
  pdf_free(&doc->pdf);
  for (gizmo = doc->gizmos; gizmo; gizmo = next_gizmo) {
    next_gizmo = gizmo->next;
    free(gizmo);
  }
}

static void
build_document(struct document *doc)
{
  struct pdf_pages pages;
  struct pdf_content content;
  struct pdf_obj_indirect *content_ref, *catalogue_ref;
  struct pdf_obj *resources;
  struct gizmo *gizmo;
  struct gizmo_text *text;
  struct gizmo_image *image;
  struct gizmo_glue *glue;
  int height;
  catalogue_ref = pdf_allocate_indirect_obj(&doc->pdf);
  pdf_pages_init(&doc->pdf, &pages);
  pdf_content_init(&content);
  height = 842 - top_margin;
  for (gizmo = doc->gizmos; gizmo; gizmo = gizmo->next) {
    switch (gizmo->type) {
    case GIZMO_TEXT:
      text = (struct gizmo_text *)gizmo;
      height -= text->font_size;
      pdf_content_write_text(&content, text->str, left_margin, height, text->font_size);
      break;
    case GIZMO_IMAGE:
      image = (struct gizmo_image *)gizmo;
      height -= image->h;
      pdf_content_write_image(&content, image->name, left_margin, height, image->w, image->h);
      break;
    case GIZMO_GLUE:
      glue = (struct gizmo_glue *)gizmo;
      if (glue->is_optimal) {
        content_ref = pdf_allocate_indirect_obj(&doc->pdf);
        pdf_content_define(&doc->pdf, content_ref, &content);
        pdf_pages_add_page(&doc->pdf, &pages, content_ref);
        pdf_content_reset_page(&content);
        height = 842 - top_margin;
      } else {
        height -= glue->no_break_height;
      }
      break;
    default:
      fprintf(stderr, "tw: Unknown gizmo type %d.\n", gizmo->type);
      exit(1);
    }
  }
  content_ref = pdf_allocate_indirect_obj(&doc->pdf);
  pdf_content_define(&doc->pdf, content_ref, &content);
  pdf_pages_add_page(&doc->pdf, &pages, content_ref);
  pdf_content_free(&content);
  resources = pdf_content_create_resources(&doc->pdf, doc->xobjects);
  pdf_pages_define_catalogue(&doc->pdf, catalogue_ref, &pages, resources);
  pdf_pages_free(&pages);
}

static void
put_text(struct document *doc, const char *str)
{
  struct gizmo_text *text;
  text = xmalloc(sizeof(struct gizmo_text));
  text->type = GIZMO_TEXT;
  text->next = NULL;
  text->font_size = font_size;
  text->str = str;
  *doc->gizmos_end = (struct gizmo *)text;
  doc->gizmos_end = &text->next;
}

static void
put_glue(struct document *doc, int break_penalty, int no_break_height)
{
  struct gizmo_glue *glue;
  glue = xmalloc(sizeof(struct gizmo_glue));
  glue->type = GIZMO_GLUE;
  glue->next = NULL;
  glue->break_penalty = break_penalty;
  glue->no_break_height = no_break_height;
  glue->best_source = NULL;
  glue->best_total_penalty = 0;
  glue->is_optimal = 0;
  *doc->gizmos_end = (struct gizmo *)glue;
  doc->gizmos_end = &glue->next;
}

static int
read_line(FILE *file, char **line, int *allocated)
{
  int c, len;
  len = 0;
  while ( (c = fgetc(file)) != EOF) {
    if (len >= *allocated) {
      *allocated += 256;
      *line = xrealloc(*line, *allocated);
    }
    switch (c) {
    case '\r':
      continue;
    case '\n':
      (*line)[len] = '\0';
      return 1;
    }
    (*line)[len++] = c;
  }
  (*line)[len] = '\0';
  return 0;
}

static void
read_file(struct document *doc, FILE *file)
{
  char *line, *str;
  int line_allocated;

  line_allocated = 256;
  line = xmalloc(line_allocated);
  while (read_line(stdin, &line, &line_allocated)) {
    str = stralloc_alloc(&stralloc, line);
    if (line[0] == '\0') {
      put_glue(doc, 0, font_size);
      continue;
    }
    put_text(doc, str);
    put_glue(doc, font_size * 12, 0);
  }
  free(line);
}

int
main(int argc, char **argv)
{
  struct document doc;
  stralloc_init(&stralloc);
  init_document(&doc);

  read_file(&doc, stdin);

  optimise_breaks(&doc);
  build_document(&doc);
  pdf_write(&doc.pdf, "output.pdf");

  free_document(&doc);
  stralloc_free(&stralloc);
  return 0;
}
