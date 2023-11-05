/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "twpdf.h"
#include "twcontent.h"
#include "twjpeg.h"
#include "twpages.h"
#include "document.h"

static void relax_glue(struct gizmo_glue *start, struct gizmo_glue *sentinel, int max_height);

static void
relax_glue(struct gizmo_glue *start, struct gizmo_glue *sentinel, int max_height)
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
      if (used_height > max_height) {
        total_penalty += 10000;
        stop = 1;
      } else {
        total_penalty += max_height - used_height;
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
    if (used_height > max_height)
      total_penalty += 10000;
    if (sentinel->best_source == NULL || sentinel->best_total_penalty > total_penalty) {
      sentinel->best_source = start;
      sentinel->best_total_penalty = total_penalty;
    }
  }
}

int
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


void
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
  relax_glue(&start_gizmo, sentinel_gizmo, 842 - doc->top_margin - doc->bot_margin);
  for (gizmo = doc->gizmos; gizmo != (struct gizmo *)sentinel_gizmo; gizmo = gizmo->next)
    if (gizmo->type == GIZMO_GLUE)
      relax_glue((struct gizmo_glue *)gizmo, sentinel_gizmo, 842 - doc->top_margin - doc->bot_margin);
  for (glue = sentinel_gizmo; glue != &start_gizmo; glue = glue->best_source)
    glue->is_optimal = 1;
  *doc->gizmos_end = NULL;
  free(sentinel_gizmo);
}

void
init_document(struct document *doc, int top_margin, int bot_margin, int left_margin)
{
  pdf_init_empty(&doc->pdf);
  doc->top_margin = top_margin;
  doc->bot_margin = bot_margin;
  doc->left_margin = left_margin;
  doc->xobjects = pdf_create_dictionary(&doc->pdf);
  doc->gizmos = NULL;
  doc->gizmos_end = &doc->gizmos;
}

void
free_document(struct document *doc)
{
  struct gizmo *gizmo, *next_gizmo;
  pdf_free(&doc->pdf);
  for (gizmo = doc->gizmos; gizmo; gizmo = next_gizmo) {
    next_gizmo = gizmo->next;
    free(gizmo);
  }
}

void
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
  height = 842 - doc->top_margin;
  for (gizmo = doc->gizmos; gizmo; gizmo = gizmo->next) {
    switch (gizmo->type) {
    case GIZMO_TEXT:
      text = (struct gizmo_text *)gizmo;
      height -= text->font_size;
      pdf_content_write_text(&content, text->str, doc->left_margin, height, text->font_size);
      break;
    case GIZMO_IMAGE:
      image = (struct gizmo_image *)gizmo;
      height -= image->h;
      pdf_content_write_image(&content, image->name, doc->left_margin, height, image->w, image->h);
      break;
    case GIZMO_GLUE:
      glue = (struct gizmo_glue *)gizmo;
      if (glue->is_optimal) {
        content_ref = pdf_allocate_indirect_obj(&doc->pdf);
        pdf_content_define(&doc->pdf, content_ref, &content);
        pdf_pages_add_page(&doc->pdf, &pages, content_ref);
        pdf_content_reset_page(&content);
        height = 842 - doc->top_margin;
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

void
put_text(struct document *doc, const char *str, int font_size)
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

void
put_image(struct document *doc, const char *fname, int w)
{
  struct pdf_jpeg_info image_info;
  struct gizmo_image *image;
  doc->xobjects = pdf_prepend_dictionary(&doc->pdf, doc->xobjects, fname,
      (struct pdf_obj *)pdf_jpeg_define(&doc->pdf, fname, &image_info));
  image = xmalloc(sizeof(struct gizmo_image));
  image->type = GIZMO_IMAGE;
  image->next = NULL;
  image->w = w;
  image->h = ((float)w / (float)image_info.width) * (float)image_info.height;
  image->name = fname;
  *doc->gizmos_end = (struct gizmo *)image;
  doc->gizmos_end = &image->next;
}

void
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
