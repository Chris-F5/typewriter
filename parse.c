#include "tw.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct symbol_parser {
  int symbol;
  int *parser;
};

struct parse_error {
  int opcode;
  const char *location;
};

typedef void (*Parser)(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);

static void parse(const int **operation, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_char(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_char_range(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_grammar(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_symbol(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_symbol_string(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_choice(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_seq(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_any(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_some(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);
static void parse_optional(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error);

const Parser parsers[] = {
  [PARSE_CHAR] = parse_char,
  [PARSE_CHAR_RANGE] = parse_char_range,
  [PARSE_GRAMMAR] = parse_grammar,
  [PARSE_SYMBOL] = parse_symbol,
  [PARSE_SYMBOL_STRING] = parse_symbol_string,
  [PARSE_CHOICE] = parse_choice,
  [PARSE_SEQ] = parse_seq,
  [PARSE_ANY] = parse_any,
  [PARSE_SOME] = parse_some,
  [PARSE_OPTIONAL] = parse_optional,
};

static void
parse_char(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  if (*input == NULL)
    goto end;
  if ((*operands)[0] == **input)
    (*input)++;
  else
    *input = NULL;
end:
  *operands += 1;
}

static void
parse_char_range(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  if (*input == NULL)
    goto end;
  if (**input >= (*operands)[0] && **input <= (*operands)[1])
    (*input)++;
  else
    *input = NULL;
end:
  *operands += 1;
}

static void
parse_grammar(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  const int *new_operation;
  if (*input == NULL)
    goto end;
  new_operation = grammar[(*operands)[0]];
  parse(&new_operation, input, next_symbol, symbol_stack, error);
end:
  *operands += 1;
}

static void
parse_symbol(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  struct symbol **next_child_symbol;
  if (*input == NULL) {
    *operands += 1;
    parse(operands, NULL, NULL, NULL, NULL);
    return;
  }
  **next_symbol = stack_allocate(symbol_stack, sizeof(struct symbol));
  (**next_symbol)->type = (*operands)[0];
  (**next_symbol)->str_len = 0;
  (**next_symbol)->str = NULL;
  (**next_symbol)->children = NULL;
  (**next_symbol)->next = NULL;
  *operands += 1;
  next_child_symbol = &(**next_symbol)->children;
  parse(operands, input, &next_child_symbol, symbol_stack, error);
  *next_symbol = &(**next_symbol)->next;
}

static void
parse_symbol_string(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  struct symbol **next_child_symbol;
  if (*input == NULL) {
    *operands += 1;
    parse(operands, NULL, NULL, NULL, NULL);
    return;
  }
  **next_symbol = stack_allocate(symbol_stack, sizeof(struct symbol));
  (**next_symbol)->type = (*operands)[0];
  (**next_symbol)->str = *input;
  (**next_symbol)->children = NULL;
  (**next_symbol)->next = NULL;
  *operands += 1;
  next_child_symbol = &(**next_symbol)->children;
  parse(operands, input, &next_child_symbol, symbol_stack, error);
  /*
   * If `*input` is NULL then `**next_symbol` will be discarded by the calling
   * `parse` so it does not matter that `str_len` is set negative.
   */
  (**next_symbol)->str_len = *input - (**next_symbol)->str;
  *next_symbol = &(**next_symbol)->next;
}

static void
parse_choice(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  const char *base_input;
  base_input = *input;
  while (**operands != END_PARSE)
    if (base_input) {
      *input = base_input;
      parse(operands, input, next_symbol, symbol_stack, NULL);
      if (*input)
        base_input = NULL;
    } else {
      parse(operands, NULL, NULL, NULL, NULL);
    }
  (*operands) += 1;
}

static void
parse_seq(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  while (**operands != END_PARSE)
    if (*input)
      parse(operands, input, next_symbol, symbol_stack, error);
    else
      parse(operands, NULL, NULL, NULL, NULL);
  (*operands) += 1;
}

static void
parse_any(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  const int *op;
  const char *last_input;
  op = *operands;
  while (*input) {
    last_input = *input;
    *operands = op;
    parse(operands, input, next_symbol, symbol_stack, NULL);
  }
  *input = last_input;
}

static void
parse_some(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  const int *op;
  op = *operands;
  parse(operands, input, next_symbol, symbol_stack, error);
  if (*input) {
    *operands = op;
    parse_any(operands, input, next_symbol, symbol_stack, error);
  }
}

static void
parse_optional(const int **operands, const char **input,
    struct symbol ***next_symbol, struct stack *symbol_stack,
    struct parse_error *error)
{
  const char *base_input;
  base_input = *input;
  parse(operands, input, next_symbol, symbol_stack, NULL);
  if (*input == NULL)
    *input = base_input;
}

static void
parse(const int **operation, const char **input, struct symbol ***next_symbol,
    struct stack *symbol_stack, struct parse_error *error)
{

  int opcode;
  Parser parser;
  int stack_height_base;
  const char *input_base;
  struct symbol **next_symbol_base;

  opcode = *(*operation)++;
  if (opcode >= sizeof(parsers) / sizeof(parsers[0])) {
    fprintf(stderr, "Unrecognised parse opcode %d\n", opcode);
    return;
  }
  parser = parsers[opcode];
  if (parser == NULL) {
    fprintf(stderr, "Unrecognised parse opcode %d\n", opcode);
    return;
  }

  if (input == NULL || *input == NULL) {
    parser(operation, &(const char *){NULL}, NULL, NULL, NULL);
    return;
  }

  stack_height_base = symbol_stack->height;
  next_symbol_base = *next_symbol;
  input_base = *input;

  parser(operation, input, next_symbol, symbol_stack, error);

  if (*input == NULL) {
    *next_symbol = next_symbol_base;
    **next_symbol = NULL;
    stack_free(symbol_stack, stack_height_base);
    if (error) {
      error->opcode = opcode;
      error->location = input_base;
    }
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
  sym = sym->children;
  while (sym) {
    print_symbol_tree(sym, indent + 1);
    sym = sym->next;
  }
}

struct symbol *
parse_document(const char *document, struct stack *sym_stack)
{
  struct symbol *root_sym;
  struct parse_error parse_error;
  const char *input;
  const int *operation;
  struct symbol **next_child;

  parse_error.opcode = 0;
  parse_error.location = NULL;

  input = document;
  operation = grammar[GRAMMAR_ROOT];
  root_sym = stack_allocate(sym_stack, sizeof(struct symbol));
  root_sym->type = SYMBOL_ROOT;
  root_sym->str_len = 0;
  root_sym->str = 0;
  root_sym->children = NULL;
  root_sym->next = NULL;
  next_child = &root_sym->children;
  parse(&operation, &input, &next_child, sym_stack, &parse_error);
  if (!input) {
    fprintf(stderr, "Failed to parse document. %s\n", parse_error.location);
    return NULL;
  }
  if (*input != '\0') {
    fprintf(stderr,
        "Failed to parse document at: \n== START ==\n%s=== END ===\n", input);
    return NULL;
  }
  return root_sym;
}
