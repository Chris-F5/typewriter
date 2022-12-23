#include "tw.h"

static struct style_node *style_symbol(const struct symbol *sym,
    struct stack *style_node_stack);

static struct style symbol_styles[] = {
  [SYMBOL_DOCUMENT] = {
    0, 0, 0, 0,
    0,
  },
  [SYMBOL_PARAGRAPH] = {
    0, 0, 0, 0,
    0,
  },
  [SYMBOL_REGULAR_WORD] = {
    0, 0, 0, 0,
    12,
  },
  [SYMBOL_BOLD_WORD] = {
    0, 0, 0, 0,
    12,
  },
};

static struct style_node *
style_symbol(const struct symbol *sym, struct stack *style_node_stack)
{
  struct style_node *style_node;
  const struct symbol *sym_child;
  style_node = stack_push(style_node_stack);
  style_node->style = symbol_styles[sym->type];
  style_node->str_len = sym->str_len;
  style_node->str = sym->str;

  for (
      sym_child = sym->child_first;
      sym_child;
      sym_child = sym_child->next_sibling) {
    if (style_node->child_last) {
      style_node->child_last = style_node->child_last->next_sibling
        = style_symbol(sym_child, style_node_stack);
    } else {
      style_node->child_last = style_node->child_first
        = style_symbol(sym_child, style_node_stack);
    }
  }

  return style_node;
}

void
print_style_tree(struct style_node* style_node, int indent)
{
  int i;
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("font_size: %d\n", style_node->style.font_size);
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
  return style_symbol(root_sym, style_node_stack);
}
