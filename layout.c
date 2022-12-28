#include "tw.h"

#include <string.h>

struct layout_resources {
  const struct font_info *font;
};

static struct layout_box *layout_adjacent(struct style_node *node,
    struct stack *fragment_stack, int max_width, int max_height,
    struct stack *layout_stack, const struct layout_resources *resources,
    int horizontal);
static struct layout_box *layout_vertical(struct style_node *node,
    struct stack *fragment_stack, int max_width, int max_height,
    struct stack *layout_stack, const struct layout_resources *resources);
static struct layout_box *layout_horizontal(struct style_node *node,
    struct stack *fragment_stack, int max_width, int max_height,
    struct stack *layout_stack, const struct layout_resources *resources);
static struct layout_box *layout_word(struct style_node *node,
    struct stack *fragment_stack, int max_width, int max_height,
    struct stack *layout_stack, const struct layout_resources *resources);
static struct layout_box * layout(struct style_node *node,
    struct stack *fragment_stack, int max_width, int max_height,
    struct stack *layout_stack, const struct layout_resources *resources);

static struct layout_box *(*display_layouts[])(struct style_node *node,
    struct stack *fragment_stack, int max_width, int max_height,
    struct stack *layout_stack, const struct layout_resources *resources) = {
  [LAYOUT_VERTICAL] = layout_vertical,
  [LAYOUT_HORIZONTAL] = layout_horizontal,
  [LAYOUT_WORD] = layout_word,
};

static struct layout_box *
layout_adjacent(struct style_node *node, struct stack *fragment_stack,
    int max_width, int max_height, struct stack *layout_stack,
    const struct layout_resources *resources, int horizontal)
{
  int content_width, content_height, x, y, highest, widest;
  struct style_node *child_node;
  struct layout_box *block_box;

  block_box = stack_push(layout_stack);
  child_node = stack_pop_pointer(fragment_stack);
  if (child_node == NULL)
    child_node = node->child_first;

  content_width = max_width - node->style->padding_left
    - node->style->padding_right;
  content_height = max_height - node->style->padding_top
    - node->style->padding_bottom;
  x = y = 0;
  highest = widest = 0;

  block_box->str_len = 0;
  block_box->str = NULL;
  block_box->child_first = block_box->child_last
    = layout(child_node, fragment_stack, content_width, content_height,
        layout_stack, resources);
  if (block_box->child_first == NULL) {
    stack_pop(layout_stack);
    return NULL; /* insufficient space for any children */
  }
  for (;;) {
    block_box->child_last->x = node->style->padding_left + x;
    block_box->child_last->y = node->style->padding_top + y;
    if (block_box->child_last->width > widest)
      widest = block_box->child_last->width;
    if (block_box->child_last->width > highest)
      highest = block_box->child_last->height;
    if (horizontal)
      x += block_box->child_last->width;
    else
      y += block_box->child_last->height;
    if (fragment_stack->height == 0)
      child_node = child_node->next_sibling;
    if (child_node == NULL)
      break; /* consumed all content */
    block_box->child_last->next_sibling = layout(child_node, fragment_stack,
        content_width - x, content_height - y, layout_stack,
        resources);
    if (block_box->child_last->next_sibling == NULL) {
      stack_push_pointer(fragment_stack, child_node);
      break; /* insufficient space for this child */
    }
    if (horizontal)
      x += block_box->child_last->next_sibling->margin_right
        > block_box->child_last->margin_left
        ? block_box->child_last->next_sibling->margin_right
        : block_box->child_last->margin_left;
    else
      y += block_box->child_last->next_sibling->margin_bottom
        > block_box->child_last->margin_top
        ? block_box->child_last->next_sibling->margin_bottom
        : block_box->child_last->margin_top;

    block_box->child_last = block_box->child_last->next_sibling;
  }
  block_box->width = node->style->padding_left + node->style->padding_right
    + (horizontal ? x : widest);
  block_box->height = node->style->padding_top + node->style->padding_bottom
    + (horizontal ? highest : y);
  block_box->margin_top = node->style->margin_top;
  block_box->margin_bottom = node->style->margin_bottom;
  block_box->margin_left = node->style->margin_left;
  block_box->margin_right = node->style->margin_right;
  return block_box;
}

static struct layout_box *
layout_vertical(struct style_node *node, struct stack *fragment_stack,
    int max_width, int max_height, struct stack *layout_stack,
    const struct layout_resources *resources)
{
  return layout_adjacent(node, fragment_stack, max_width, max_height,
      layout_stack, resources, 0);
}

static struct layout_box *
layout_horizontal(struct style_node *node, struct stack *fragment_stack,
    int max_width, int max_height, struct stack *layout_stack,
    const struct layout_resources *resources)
{
  return layout_adjacent(node, fragment_stack, max_width, max_height,
      layout_stack, resources, 1);
}

static struct layout_box *
layout_word(struct style_node *node, struct stack *fragment_stack,
    int max_width, int max_height, struct stack *layout_stack,
    const struct layout_resources *resources)
{
  struct layout_box *box;
  int i, width, height;
  width = 0;
  for (i = 0; i < node->str_len; i++)
    width += resources->font->char_widths[node->str[i]];
  width *= node->style->font_size;
  width /= 1000;
  width += node->style->padding_left + node->style->padding_right;
  height = node->style->padding_top + node->style->padding_bottom
    + node->style->font_size;
  if (height > max_height)
    return NULL;
  if (width > max_width)
    return NULL;
  box = stack_push(layout_stack);
  box->width = width;
  box->height = height;
  box->str_len = node->str_len;
  box->str = node->str;
  box->child_first = box->child_last = NULL;
  box->next_sibling = NULL;
  box->margin_top = node->style->margin_top;
  box->margin_bottom = node->style->margin_bottom;
  box->margin_left = node->style->margin_left;
  box->margin_right = node->style->margin_right
    + resources->font->char_widths[' '] * node->style->font_size / 1000;
  return box;
}

static struct layout_box *
layout(struct style_node *node, struct stack *fragment_stack, int max_width,
    int max_height, struct stack *layout_stack,
    const struct layout_resources *resources)
{
  return display_layouts[node->style->display_type]
    (node, fragment_stack, max_width, max_height, layout_stack, resources);
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
layout_pages(struct style_node *root_style_node,
    struct stack *layout_stack, const struct font_info *font)
{
  struct stack fragment_stack;
  struct layout_resources resources;
  struct layout_box *page;

  stack_init(&fragment_stack, 32, sizeof(struct style_node *));

  resources.font = font;

  page = layout(root_style_node, &fragment_stack, 596, 842, layout_stack,
      &resources);
  page->x = 0;
  page->y = 0;
  page->height = 842;
  return page;
}
