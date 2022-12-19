#include "tw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum parse_types {
  PARSE_PATTERN,
  PARSE_OR,
  PARSE_SOME,
};

enum token_names {
  TOKEN_ROOT,
  TOKEN_PARAGRAPH,
  TOKEN_ANY_WORD,
  TOKEN_REGULAR_WORD,
  TOKEN_BOLD_WORD,
};

struct parser {
  int type, symbol;
  char *start, *middle, *end;
  struct parser **parsers;
};

static int set_match(const char **set, char c);
static const char *pattern_match(const char *pattern, const char *input);
static struct symbol *push_symbol(struct symbol_stack *stack);
static void pop_symbol(struct symbol_stack *stack);
static const char *parse(struct parser *parser, const char *input,
    struct symbol *sym, struct symbol_stack *stack);

struct parser parsers[] = {
  [TOKEN_ROOT] = {
    PARSE_SOME, SYMBOL_DOCUMENT,
    "*\n", "*\n", "*[ \n]",
    (struct parser *[]) {
      &parsers[TOKEN_PARAGRAPH],
    }
  },
  [TOKEN_PARAGRAPH] = {
    PARSE_SOME, SYMBOL_PARAGRAPH,
    "", "* ?\n", "",
    (struct parser *[]) {
      &parsers[TOKEN_ANY_WORD],
    }
  },
  [TOKEN_ANY_WORD] = {
    PARSE_OR, 0,
    "", "", "",
    (struct parser *[]) {
      &parsers[TOKEN_BOLD_WORD],
      &parsers[TOKEN_REGULAR_WORD],
      NULL
    }
  },
  [TOKEN_REGULAR_WORD] = {
    PARSE_PATTERN, SYMBOL_REGULAR_WORD,
    "", "-!~*-!~", "",
    NULL
  },
  [TOKEN_BOLD_WORD] = {
    PARSE_PATTERN, SYMBOL_BOLD_WORD,
    "\\*", "-!~*-!~", "\\*",
    NULL
  },
};

static int
set_match(const char **set, char c)
{
  char start, end;
  int match;
  switch (**set) {
  case '\\':
    (*set)++;
    return c == *(*set)++;
  case '-':
    (*set)++;
    start = *(*set)++;
    end = *(*set)++;
    return c >= start && c <= end;
  case '[':
    (*set)++;
    match = 0;
    while (**set != ']')
      if (set_match(set, c)) match = 1;
    (*set)++;
    return match;
  default:
    return c == *(*set)++;
  }
}

/* If pattern contains an error, behaviour is undefined. */
static const char *
pattern_match(const char *pattern, const char *input)
{
  const char *base;
  if (input == NULL) return NULL;
  while (*pattern) {
    switch (*pattern) {
    case '*':
      base = pattern++;
      if (set_match(&pattern, *input)) {
        input++;
        pattern = base;
      }
      break;
    case '?':
      pattern++;
      if (set_match(&pattern, *input))
        input++;
      break;
    default:
      if (set_match(&pattern, *input))
        input++;
      else
        return NULL;
      break;
    }
  }
  return input;
}

static struct symbol *
push_symbol(struct symbol_stack *stack)
{
  struct symbol_stack_page *new_page;
  if (stack->page_full == SYMBOL_STACK_PAGE_SIZE) {
    new_page = xmalloc(sizeof(struct symbol_stack_page));
    new_page->below = stack->top_page;
    stack->top_page = new_page;
    stack->page_full = 0;
  }
  return &stack->top_page->symbols[stack->page_full++];
}

static void
pop_symbol(struct symbol_stack *stack)
{
  struct symbol_stack_page *old_page;
  if (stack->page_full == 0) {
    old_page = stack->top_page;
    stack->top_page = stack->top_page->below;
    if (stack->top_page == NULL)
      fprintf(stderr, "Can't pop from empty symbol stack.");
    free(old_page);
    stack->page_full = SYMBOL_STACK_PAGE_SIZE;
  }
  stack->page_full--;
}

static void
free_symbol_stack(struct symbol_stack *stack)
{
  struct symbol_stack_page *old_page;
  while (stack->top_page) {
    old_page = stack->top_page;
    stack->top_page = stack->top_page->below;
    free(old_page);
  }
}

static const char *
parse(struct parser *parser, const char *input, struct symbol *sym,
    struct symbol_stack *stack)
{
  struct parser **child_parser;
  const char *parse_result;
  struct symbol **next_sym;
  int symbols_pushed;
  if (input == NULL) return NULL;
  switch (parser->type) {
    case PARSE_PATTERN:
      sym->type = parser->symbol;
      sym->children = NULL;
      input = pattern_match(parser->start, input);
      sym->str = input;
      input = pattern_match(parser->middle, input);
      sym->str_len = input - sym->str;
      input = pattern_match(parser->end, input);
      return input;
    case PARSE_OR:
      for (
          child_parser = parser->parsers;
          *child_parser != NULL;
          child_parser += 1) {
        parse_result = parse(*child_parser, input, sym, stack);
        if (parse_result) return parse_result;
      }
      return NULL;
    case PARSE_SOME:
      sym->type = parser->symbol;
      sym->str_len = 0;
      sym->str = NULL;
      next_sym = &sym->children;
      input = pattern_match(parser->start, input);
      symbols_pushed = 0;
      for (;;) {
        *next_sym = push_symbol(stack);
        symbols_pushed++;
        parse_result = parse(parser->parsers[0], input, *next_sym, stack);
        if (parse_result == NULL) {
          pop_symbol(stack);
          symbols_pushed--;
          break;
        }
        input = parse_result;
        next_sym = &((**next_sym).next_sibling);

        parse_result = pattern_match(parser->middle, input);
        if (parse_result == NULL)
          break;
        input = parse_result;
      }
      *next_sym = NULL;
      input = pattern_match(parser->end, input);
      if (sym->children == NULL) {
        for (; symbols_pushed; symbols_pushed--)
          pop_symbol(stack);
        return NULL;
      }
      return input;
    default:
      fprintf(stderr, "Invalid parser type!\n");
      return NULL;
  }
}

void print_symbol_tree(struct symbol* sym, int indent)
{
  int i;
  for (i = 0; i < indent; i++) putchar(' ');
  printf("%d\n", sym->type);
  if (sym->str) {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    putchar('"');
    fwrite(sym->str, 1, sym->str_len, stdout);
    putchar('"');
    putchar('\n');
  }
  sym = sym->children;
  while (sym) {
    print_symbol_tree(sym, indent + 1);
    sym = sym->next_sibling;
  }
}

struct symbol *
parse_document(const char *document, struct symbol_stack *stack)
{
  struct symbol *root_sym;

  stack->page_full = 0;
  stack->top_page = xmalloc(sizeof(struct symbol_stack_page));
  stack->top_page->below = NULL;

  root_sym = push_symbol(stack);
  document = parse(&parsers[TOKEN_ROOT], document, root_sym, stack);
  if (document == NULL) {
    fprintf(stderr, "Failed to parse document.\n");
    return NULL;
  }
  if (*document != '\0') {
    fprintf(stderr, "Failed to parse all document: %s\n", document);
    fprintf(stderr, "Failed to parse all document: %d\n", *document);
    return NULL;
  }
  return root_sym;
}
