#include "tw.h"

#include <string.h>

/*
static void (*symbol_layouts[])(int max_width, int max_height,
    const struct symbol **sym_path, struct stack *layout_stack) = {
  [SYMBOL_PARAGRAPH] = layout_vertical_block,
  [SYMBOL_REGULAR_WORD] = layout_word,
};
*/

static struct layout_box *layout_block(int max_width, int max_height,
    const struct style_node **path, struct stack *layout_stack);
static struct layout_box *layout_word(int max_width, int max_height,
    const struct style_node **path, struct stack *layout_stack);
static struct layout_box *layout(int max_width, int max_height,
    const struct style_node **path, struct stack *layout_stack);

static struct layout_box *
layout_block(int max_width, int max_height,
    const struct style_node **path, struct stack *layout_stack)
{
  struct layout_box *box, *child_box;
  int x, y, max_x, max_y;

  if (path[1] == NULL)
    path[1] = path[0]->child_first;

  box = stack_push(layout_stack);
  box->str_len = 0;
  box->str = NULL;
  box->child_first = box->child_last = NULL;
  box->next_sibling = NULL;

  x = path[0]->style.margin_left;
  y = path[0]->style.margin_top;
  max_x = max_width - path[0]->style.margin_right;
  max_y = max_height - path[0]->style.margin_bottom;
  while (path[1]) {
    child_box = layout(max_x - x, max_y - y, path + 1, layout_stack);
    if (child_box == NULL) break;
    child_box->x = x;
    child_box->y = y;
    y += child_box->height;
    if (box->child_last)
      box->child_last = box->child_last->next_sibling = child_box;
    else
      box->child_last = box->child_first = child_box;
    if (path[2]) break;
    path[1] = path[1]->next_sibling;
  }
  box->width = max_width;
  box->height = y + path[0]->style.margin_bottom;
  return box;
}

static struct layout_box *
layout_word(int max_width, int max_height,
    const struct style_node **path, struct stack *layout_stack)
{
  struct layout_box *box;
  if (max_height < path[0]->style.font_size)
    return NULL;
  box = stack_push(layout_stack);
  box->width = max_width; /* todo: read character width */
  box->height = path[0]->style.font_size;
  box->str_len = path[0]->str_len;
  box->str = path[0]->str;
  box->child_first = box->child_last = NULL;
  box->next_sibling = NULL;
  return box;
}

static struct layout_box *
layout(int max_width, int max_height,
    const struct style_node **path, struct stack *layout_stack)
{
  if (path[0]->child_first == NULL)
    return layout_word(max_width, max_height, path, layout_stack);
  else
    return layout_block(max_width, max_height, path, layout_stack);
}

void
print_layout_tree(const struct layout_box *layout, int indent)
{
  int i;
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("x: %d\n", layout->x);
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("y: %d\n", layout->y);
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("w: %d\n", layout->width);
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("h: %d\n", layout->height);
  if (layout->str) {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    putchar('"');
    fwrite(layout->str, 1, layout->str_len, stdout);
    putchar('"');
    putchar('\n');
  }
  layout = layout->child_first;
  while (layout) {
    print_layout_tree(layout, indent + 1);
    layout = layout->next_sibling;
  }
}

struct layout_box *
layout_pages(const struct style_node *root_style_node,
    struct stack *layout_stack)
{
  const struct style_node *path[32];
  struct layout_box *page;

  memset(path, 0, sizeof(path));
  path[0] = root_style_node;

  page = layout(596, 842, path, layout_stack);
  page->x = 0;
  page->y = 0;
  page->height = 842;
  return page;
}
