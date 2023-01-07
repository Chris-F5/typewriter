#include "tw.h"

static struct element *interpret_text(struct symbol *symbol,
    int text_element_type, struct stack *element_stack);
static struct element *interpret_list(struct symbol *sym, int element_type,
    struct stack *element_stack);

static struct element *
interpret_text(struct symbol *sym, int element_type,
    struct stack *element_stack)
{
  struct element *element;
  struct symbol *child_sym;
  struct span **span;
  element = stack_allocate(element_stack, sizeof(struct element));
  element->type = element_type;
  element->next = NULL;
  element->children = NULL;
  span = &element->text;
  for (
      child_sym = sym->child_first;
      child_sym;
      child_sym = child_sym->next) {
    if (child_sym->type != SYMBOL_REGULAR_WORD
        && child_sym->type != SYMBOL_BOLD_WORD)
      continue;
    *span = stack_allocate(element_stack, sizeof(struct span));
    (*span)->font_size = 12;
    (*span)->leading_char = ' ';
    (*span)->str_len = child_sym->str_len;
    (*span)->str = child_sym->str;
    if (child_sym->type == SYMBOL_BOLD_WORD)
      (*span)->font_size += 4;
    span = &(*span)->next;
  }
  *span = NULL;
  return element;
}

static struct element *
interpret_list(struct symbol *sym, int element_type,
    struct stack *element_stack)
{
  struct element *element, **child_element;
  struct symbol *child_sym;
  element = stack_allocate(element_stack, sizeof(struct element));
  element->type = element_type;
  element->text = NULL;
  element->next = NULL;
  child_element = &element->children;
  for (child_sym = sym->child_first; child_sym; child_sym = child_sym->next)
    if ( (*child_element = interpret(child_sym, element_stack)) )
      child_element = &(*child_element)->next;
  *child_element = NULL;
  return element;
}

void
print_span_list(const struct span *span)
{
  char c;
  while (span) {
    c = '\'';
    if (span->font_size > 12)
      c = '*';
    putchar(c);
    fwrite(span->str, 1, span->str_len, stdout);
    putchar(c);
    putchar(' ');
    span = span->next;
  }
  printf("\n");
}

void
print_element_tree(const struct element *element, int indent)
{
  int i;
  for (i = 0; i < indent; i++) putchar(' ');
  printf("element type: %d\n", element->type);
  if (element->text) {
    for (i = 0; i < indent; i++) putchar(' ');
    print_span_list(element->text);
  }
  element = element->children;
  while (element) {
    print_element_tree(element, indent + 2);
    element = element->next;
  }
}

struct element *
interpret(struct symbol *sym, struct stack *element_stack)
{
  switch (sym->type) {
  case SYMBOL_NONE:
    return NULL;
  case SYMBOL_DOCUMENT:
    return interpret_list(sym, ELEMENT_PAGE, element_stack);
  case SYMBOL_PARAGRAPH:
    return interpret_text(sym, ELEMENT_TEXT_JUSTIFIED, element_stack);
  case SYMBOL_REGULAR_WORD: /* fallthrough */
  case SYMBOL_BOLD_WORD:
    fprintf(stderr, "Can't interpret isolated word\n");
    return NULL;
  default:
    fprintf(stderr, "Can't interpret symbol type '%d'\n", sym->type);
    return NULL;
  }
}
