#include "tw.h"

struct symbol_style_mapping {
  struct style style;
  struct symbol_style_mapping *child;
};

static struct style_node *symbol_to_style(const struct symbol *sym,
    struct stack *style_node_stack, const struct symbol_style_mapping *mapping);

static const struct symbol_style_mapping symbol_style_map[] = {
  [SYMBOL_DOCUMENT] = {
    {
      LAYOUT_VERTICAL,
      60, 60, 70, 70,
      0, 0, 0, 0,
      0,
    },
    NULL
  },
  [SYMBOL_PARAGRAPH] = {
    { /* paragraph */
      LAYOUT_VERTICAL,
      0, 0, 0, 0,
      15, 15, 0, 0,
      0,
    },
    &(struct symbol_style_mapping){
      { /* line */
        LAYOUT_HORIZONTAL,
        0, 0, 0, 0,
        3, 3, 0, 0,
        0,
      },
      NULL
    }
  },
  [SYMBOL_REGULAR_WORD] = {
    {
      LAYOUT_WORD,
      0, 0, 0, 0,
      0, 0, 0, 0,
      12,
    },
    NULL
  },
  [SYMBOL_BOLD_WORD] = {
    {
      LAYOUT_WORD,
      0, 0, 0, 0,
      0, 0, 0, 0,
      12,
    },
    NULL
  },
};

static struct style_node *
symbol_to_style(const struct symbol *sym, struct stack *style_node_stack,
    const struct symbol_style_mapping *mapping)
{
  struct style_node *style_node;
  const struct symbol *sym_child;
  struct style_node *child_node;

  style_node = stack_push(style_node_stack);
  style_node->style = &mapping->style;
  style_node->next_sibling = NULL;

  if (mapping->child) {
    style_node->child_first = style_node->child_last
      = symbol_to_style(sym, style_node_stack, mapping->child);
    return style_node;
  }

  style_node->str_len = sym->str_len;
  style_node->str = sym->str;


  sym_child = sym->child_first;
  if (!sym_child)
    return style_node;
  child_node = symbol_to_style(sym_child, style_node_stack,
    &symbol_style_map[sym_child->type]);
  if (!child_node)
    return style_node;
  style_node->child_last = style_node->child_first = child_node;
  sym_child = sym_child->next_sibling;
  while (sym_child) {
    child_node = symbol_to_style(sym_child, style_node_stack,
        &symbol_style_map[sym_child->type]);
    if (child_node)
      style_node->child_last = style_node->child_last->next_sibling
        = child_node;
    sym_child = sym_child->next_sibling;
  }

  return style_node;
}

void
print_style_tree(struct style_node* style_node, int indent)
{
  int i;
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("font_size: %d\n", style_node->style->font_size);
  if (style_node->str) {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    putchar('"');
    fwrite(style_node->str, 1, style_node->str_len, stdout);
    putchar('"');
    putchar('\n');
  }
  style_node = style_node->child_first;
  while (style_node) {
    print_style_tree(style_node, indent + 1);
    style_node = style_node->next_sibling;
  }
}

struct style_node *
create_style_tree(const struct symbol *root_sym, struct stack *style_node_stack)
{
  return symbol_to_style(root_sym, style_node_stack,
      &symbol_style_map[root_sym->type]);
}
