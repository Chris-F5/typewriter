#include <stdio.h>
#include <string.h>

#include "tw.h"

struct page_ctx {
  int x, y;
};

typedef int (*PageCommandParser)(FILE *file, struct page_ctx *page_ctx,
    struct dbuffer *text_content);

struct page_command {
  const char *str;
  PageCommandParser parser;
};

static void add_page(FILE *pdf_file, int obj_parent,
    struct pdf_xref_table *xref, struct pdf_page_list *page_list,
    const struct dbuffer *text_content);
static int parse_goto(FILE *file, struct page_ctx *page_ctx,
    struct dbuffer *text_content);
static int parse_move(FILE *file, struct page_ctx *page_ctx, struct dbuffer
    *text_content);
static int parse_text(FILE *file, struct page_ctx *page_ctx,
    struct dbuffer *text_content);

static const char *new_page_str = "PAGE";

static const struct page_command *page_commands[] = {
  &(struct page_command) {
    "GOTO",
    parse_goto,
  },
  &(struct page_command) {
    "MOVE",
    parse_move,
  },
  &(struct page_command) {
    "TEXT",
    parse_text,
  },
  NULL,
};

static void
add_page(FILE *pdf_file, int obj_parent, struct pdf_xref_table *xref,
    struct pdf_page_list *page_list, const struct dbuffer *text_content)
{
  int obj_content, obj_page;
  obj_content = allocate_pdf_obj(xref);
  obj_page = allocate_pdf_obj(xref);

  pdf_start_indirect_obj(pdf_file, xref, obj_content);
  pdf_write_text_stream(pdf_file, text_content->data, text_content->size);
  pdf_end_indirect_obj(pdf_file);

  pdf_start_indirect_obj(pdf_file, xref, obj_page);
  pdf_write_page(pdf_file, obj_parent, obj_content);
  pdf_end_indirect_obj(pdf_file);

  add_pdf_page(page_list, obj_page);
}

static int
parse_goto(FILE *file, struct page_ctx *page_ctx, struct dbuffer *text_content)
{
  fscanf(file, "%d %d\n", &page_ctx->x, &page_ctx->y);
  dbuffer_printf(text_content, "1 0 0 1 %d %d Tm\n", page_ctx->x, page_ctx->y);
  return 0;
}

static int
parse_move(FILE *file, struct page_ctx *page_ctx, struct dbuffer *text_content)
{
  int xo, yo;
  fscanf(file, "%d %d\n", &xo, &yo);
  page_ctx->x += xo;
  page_ctx->y += yo;
  dbuffer_printf(text_content, "1 0 0 1 %d %d Tm\n", page_ctx->x, page_ctx->y);
  return 0;
}

static int
parse_text(FILE *file, struct page_ctx *page_ctx,
    struct dbuffer *text_content)
{
  int c;
  while ( (c = fgetc(file)) != '\n') {
    if (c == EOF) {
      fprintf(stderr, "Unexpected EOF in text command\n");
      return 1;
    }
    dbuffer_putc(text_content, (char)c);
  }
  dbuffer_putc(text_content, '\n');
  return 0;
}

int
print_pages(FILE *pages_file, FILE *typeface_file, FILE *pdf_file)
{
  int ret, scan, i;
  char cmd_str[5];
  struct page_ctx page_ctx;
  struct dbuffer text_content;
  const struct page_command *page_cmd;
  struct pdf_xref_table xref_table;
  struct pdf_page_list page_list;
  struct pdf_resources resources;
  int obj_resources, obj_page_list, obj_catalog;
  ret = 0;
  page_ctx.x = 0;
  page_ctx.y = 0;
  dbuffer_init(&text_content, 1024 * 4, 1024 * 4);
  dbuffer_printf(&text_content, "0 0 0 rg\nBT\n");
  init_pdf_xref_table(&xref_table);
  init_pdf_page_list(&page_list);
  init_pdf_resources(&resources);
  include_font_resource(&resources, "Regular");
  include_font_resource(&resources, "Bold");

  allocate_pdf_obj(&xref_table); /* Allocate the 'zero' object. */
  pdf_write_header(pdf_file);
  obj_resources = allocate_pdf_obj(&xref_table);
  obj_page_list = allocate_pdf_obj(&xref_table);

next_command:
    scan = fscanf(pages_file, "%4s", cmd_str);
    fscanf(pages_file, " ");
    if (scan == EOF)
      goto finish_parse;
    if (scan < 1)
      cmd_str[0] = '\0';
    if (strcmp(cmd_str, new_page_str) == 0) {
      fscanf(pages_file, "\n");
      dbuffer_printf(&text_content, "ET");
      add_page(pdf_file, obj_page_list, &xref_table, &page_list,
          &text_content);
      text_content.size = 0;
      dbuffer_printf(&text_content, "0 0 0 rg\nBT\n");
      goto next_command;
    }
    i = 0;
    while ( (page_cmd = page_commands[i++]) )
      if (strcmp(cmd_str, page_cmd->str) == 0) {
        ret = page_cmd->parser(pages_file, &page_ctx, &text_content);
        if (ret)
          goto finish_parse;
        else
          goto next_command;
      }
    ret = 1;
    fprintf(stderr, "Failed to parse page command '%s'\n", cmd_str);
finish_parse:

  pdf_add_resources(pdf_file, typeface_file, obj_resources, &resources,
      &xref_table);

  pdf_start_indirect_obj(pdf_file, &xref_table, obj_page_list);
  pdf_write_pages(pdf_file, obj_resources, page_list.page_count,
      page_list.page_objs);
  pdf_end_indirect_obj(pdf_file);

  obj_catalog = allocate_pdf_obj(&xref_table);
  pdf_start_indirect_obj(pdf_file, &xref_table, obj_catalog);
  pdf_write_catalog(pdf_file, obj_page_list);
  pdf_end_indirect_obj(pdf_file);

  pdf_add_footer(pdf_file, &xref_table, obj_catalog);
  dbuffer_free(&text_content);
  return ret;
}
