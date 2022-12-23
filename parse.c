#include "tw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum sym_parser_names {
  PARSER_ROOT,
  PARSER_PARAGRAPH,
  PARSER_REGULAR_WORD,
  PARSER_BOLD_WORD,
};

enum parser_opcodes {
  /* primitives */
  PARSE_CHAR,
  PARSE_CHAR_RANGE,
  PARSE_SYMBOL,
  /* pattern */
  PARSE_CHOICE,
  PARSE_SEQ,
  PARSE_SOME,
  PARSE_ANY,
  PARSE_OPTIONAL,
  /* misc */
  PARSE_STRING,
  PARSE_END = -256
};

struct symbol_parser {
  int symbol;
  int *parser;
};

struct parser_error {
  const char *location;
};

static void parse(const int **parser, const char **input, struct symbol *sym,
    struct stack *sym_stack, struct parser_error *error);

const struct symbol_parser symbol_parsers[] = {
  [PARSER_ROOT] = {
    SYMBOL_DOCUMENT,
    (int []) {
      PARSE_SEQ,
        PARSE_ANY, PARSE_CHAR, '\n',
        PARSE_ANY, PARSE_SEQ,
          PARSE_SYMBOL, PARSER_PARAGRAPH,
          PARSE_SOME, PARSE_CHAR, '\n',
        PARSE_END,
        PARSE_ANY, PARSE_CHAR, '\n',
      PARSE_END,
    }
  },
  [PARSER_PARAGRAPH] = {
    SYMBOL_PARAGRAPH,
    (int []) {
      PARSE_SOME, PARSE_SEQ,
        PARSE_CHOICE,
          PARSE_SYMBOL, PARSER_BOLD_WORD,
          PARSE_SYMBOL, PARSER_REGULAR_WORD,
        PARSE_END,
        PARSE_CHOICE,
          PARSE_SEQ,
            PARSE_ANY, PARSE_CHAR, ' ',
            PARSE_CHAR, '\n',
          PARSE_END,
          PARSE_SOME, PARSE_CHAR, ' ',
        PARSE_END,
      PARSE_END,
    }
  },
  [PARSER_REGULAR_WORD] = {
    SYMBOL_REGULAR_WORD,
    (int []) {
      PARSE_STRING, PARSE_SOME, PARSE_CHAR_RANGE, '!', '~',
    }
  },
  [PARSER_BOLD_WORD] = {
    SYMBOL_BOLD_WORD,
    (int []) {
      PARSE_SEQ,
        PARSE_CHAR, '*',
        PARSE_STRING, PARSE_SOME, PARSE_CHOICE,
          PARSE_CHAR_RANGE, '!', ')',
          PARSE_CHAR_RANGE, '+', '~',
        PARSE_END,
        PARSE_CHAR, '*',
      PARSE_END,
    }
  },
};

static void
parse(const int **parser, const char **input, struct symbol *sym,
    struct stack *sym_stack, struct parser_error *error)
{
  int opcode;
  const char *base_input, *new_input;
  const int *base_parser, *new_parser;
  struct symbol *new_sym;
  const struct symbol_parser *new_sym_parser;

  opcode = *(*parser)++;
  switch (opcode) {

  case PARSE_CHAR:
    if (!*input) {
      (*parser) += 1;
      break;
    }
    if (**input == *(*parser)++) {
      (*input)++;
    } else {
      if (error)
        error->location = *input;
      *input = NULL;
    }
    break;

  case PARSE_CHAR_RANGE:
    if (!*input) {
      (*parser) += 2;
      break;
    }
    if (**input >= (*parser)[0] && **input <= (*parser)[1]) {
      (*input)++;
    } else {
      if (error)
        error->location = *input;
      *input = NULL;
    }
    (*parser) += 2;
    break;

  case PARSE_SYMBOL:
    if (!*input) {
      (*parser)++;
      break;
    }
    new_sym_parser = &symbol_parsers[*(*parser)++];
    new_sym = stack_push(sym_stack);
    new_sym->type = new_sym_parser->symbol;
    new_sym->str_len = 0;
    new_sym->str = NULL;
    new_sym->child_first = NULL;
    new_sym->child_last = NULL;
    new_sym->next_sibling = NULL;
    new_parser = new_sym_parser->parser;
    parse(&new_parser, input, new_sym, sym_stack, error);
    if (!*input) {
      stack_pop(sym_stack);
      break;
    }
    if (sym->child_last)
      sym->child_last = sym->child_last->next_sibling = new_sym;
    else
      sym->child_first = sym->child_last = new_sym;
    break;

  case PARSE_SEQ:
    while (**parser != PARSE_END)
      parse(parser, input, sym, sym_stack, error);
    (*parser)++;
    break;

  case PARSE_CHOICE:
    base_input = *input;
    new_input = NULL;
    while (**parser != PARSE_END) {
      parse(parser, input, sym, sym_stack, NULL);
      if (*input) {
        new_input = *input;
        *input = NULL;
      } else {
        *input = base_input;
      }
    }
    (*parser)++;
    *input = new_input;
    if (*input == NULL && error)
      error->location = base_input;
    break;

  case PARSE_SOME:
    base_parser = *parser;
    parse(parser, input, sym, sym_stack, error);
    if (!*input)
      break;
    do {
      *parser = base_parser;
      base_input = *input;
      parse(parser, input, sym, sym_stack, NULL);
    } while(*input);
    *input = base_input;
    break;

  case PARSE_ANY:
    base_parser = *parser;
    do {
      *parser = base_parser;
      base_input = *input;
      parse(parser, input, sym, sym_stack, NULL);
    } while(*input);
    *input = base_input;
    break;

  case PARSE_OPTIONAL:
    base_input = *input;
    parse(parser, input, sym, sym_stack, NULL);
    if (!*input)
      *input = base_input;
    break;

  case PARSE_STRING:
    base_input = *input;
    parse(parser, input, sym, sym_stack, error);
    sym->str = base_input;
    sym->str_len = *input - base_input;
    break;

  default:
    fprintf(stderr, "Unrecognised parse opcode! %d\n", opcode);
    break;
  }
}

void
print_symbol_tree(struct symbol* sym, int indent)
{
  int i;
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("%d\n", sym->type);
  if (sym->str) {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    putchar('"');
    fwrite(sym->str, 1, sym->str_len, stdout);
    putchar('"');
    putchar('\n');
  }
  sym = sym->child_first;
  while (sym) {
    print_symbol_tree(sym, indent + 1);
    sym = sym->next_sibling;
  }
}

struct symbol *
parse_document(const char *document, struct stack *sym_stack)
{
  struct symbol *root_sym;
  struct parser_error *parser_error;
  const char *input;
  const int *parser;

  parser_error->location = NULL;

  input = document;
  parser = symbol_parsers[PARSER_ROOT].parser;
  root_sym = stack_push(sym_stack);
  root_sym->type = symbol_parsers[PARSER_ROOT].symbol;
  root_sym->str_len = 0;
  root_sym->str = 0;
  root_sym->child_first = NULL;
  root_sym->child_last = NULL;
  root_sym->next_sibling = NULL;
  parse(&parser, &input, root_sym, sym_stack, parser_error);
  if (!input) {
    fprintf(stderr, "Failed to parse document. %s\n", parser_error->location);
    return NULL;
  }
  if (*input != '\0') {
    fprintf(stderr,
        "Failed to parse document at: \n== START ==\n%s=== END ===\n", input);
    return NULL;
  }
  return root_sym;
}
