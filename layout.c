#include "tw.h"

#include <string.h>

typedef struct pdf_graphic (*LayoutRule)(struct element *element,
    struct element_iterator *iterator, int max_width, int max_height,
    struct stack *graphics_stack);

static struct pdf_graphic layout_page(struct element *page,
    struct element_iterator *iterator, int max_width, int max_height,
    struct stack *graphics_stack);

static struct pdf_graphic layout(struct element_iterator *iterator,
    int max_width, int max_height, struct stack *graphics_stack);

static LayoutRule layout_rules[] = {
  [ELEMENT_PAGE] = layout_page,
};

static struct pdf_graphic
layout_page(struct element *page, struct element_iterator *iterator,
    int max_width, int max_height, struct stack *graphics_stack)
{
  struct pdf_graphic graphic;
  struct pdf_content_instruction* instruction;
  graphic.width = max_width;
  graphic.height = max_height;

  graphic.first = stack_push(graphics_stack);
  instruction = graphic.first;
  strcpy(instruction->operation, "rg");
  instruction->operand_count = 3;
  instruction->operands[0] = (struct pdf_primitive){PDF_NUMBER, 0};
  instruction->operands[1] = (struct pdf_primitive){PDF_NUMBER, 0};
  instruction->operands[2] = (struct pdf_primitive){PDF_NUMBER, 0};

  instruction = instruction->next = stack_push(graphics_stack);
  strcpy(instruction->operation, "BT");
  instruction->operand_count = 0;

  instruction = instruction->next = stack_push(graphics_stack);
  strcpy(instruction->operation, "Tf");
  instruction->operand_count = 2;
  instruction->operands[0] = (struct pdf_primitive){PDF_NAME, {.str = "F1"}};
  instruction->operands[1] = (struct pdf_primitive){PDF_NUMBER, 12};

  instruction = instruction->next = stack_push(graphics_stack);
  strcpy(instruction->operation, "Tw");
  instruction->operand_count = 1;
  instruction->operands[0] = (struct pdf_primitive){PDF_NUMBER, 0};

  instruction = instruction->next = stack_push(graphics_stack);
  strcpy(instruction->operation, "Td");
  instruction->operand_count = 2;
  instruction->operands[0] = (struct pdf_primitive){PDF_NUMBER, 70};
  instruction->operands[1] = (struct pdf_primitive){PDF_NUMBER, 770};

  instruction = instruction->next = stack_push(graphics_stack);
  strcpy(instruction->operation, "Tj");
  instruction->operand_count = 1;
  instruction->operands[0]
    = (struct pdf_primitive){PDF_STRING, {.str = "Hello World!"}};

  instruction = instruction->next = stack_push(graphics_stack);
  strcpy(instruction->operation, "ET");
  instruction->operand_count = 0;

  instruction->next = NULL;
  graphic.last = instruction;
  return graphic;
}

static struct pdf_graphic
layout(struct element_iterator *iterator, int max_width, int max_height,
    struct stack *graphics_stack)
{
  struct element *element;
  struct pdf_graphic graphic;
  LayoutRule layout_rule;

  element = stack_pop_pointer(&iterator->stack);
  if (element == NULL) {
    fprintf(stderr, "Can't layout NULL element.");
    return (struct pdf_graphic){0, 0, NULL, NULL};
  }
  layout_rule = layout_rules[element->type];
  if (layout_rule == NULL) {
    fprintf(stderr, "No layout rule for element type '%d'\n", element->type);
    return (struct pdf_graphic){0, 0, NULL, NULL};
  }
  graphic = layout_rule(element, iterator, max_width, max_height,
      graphics_stack);

  if (iterator->stack.height == 0)
    stack_push_pointer(&iterator->stack, element->next);
  else
    stack_push_pointer(&iterator->stack, element);

  return graphic;
}

struct pdf_graphic
layout_pdf_page(struct element *root_element, struct stack *graphics_stack)
{
  struct element_iterator iterator;
  struct pdf_graphic graphic;

  stack_init(&iterator.stack, 16, sizeof(struct element *));
  iterator.span = NULL;
  stack_push_pointer(&iterator.stack, root_element);

  graphic = layout(&iterator, 595, 824, graphics_stack);

  stack_free(&iterator.stack);
  return graphic;
}

