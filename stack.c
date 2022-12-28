#include "tw.h"

#include <stdlib.h>

void
stack_init(struct stack *stack, int page_size, int item_size)
{
  stack->item_size = item_size;
  stack->page_size = page_size;
  stack->height = 0;
  stack->top_page = NULL;
}

void *
stack_push(struct stack *stack)
{
  struct stack_page *new_page;
  if (stack->height % stack->page_size == 0) {
    new_page = xmalloc(
        sizeof(struct stack_page) + stack->page_size * stack->item_size);
    new_page->below = stack->top_page;
    stack->top_page = new_page;
  }
  return stack->top_page->data 
    + stack->item_size * (stack->height++ % stack->page_size);
}

void
stack_pop(struct stack *stack)
{
  struct stack_page *old_page;
  if (stack->height == 0) {
    fprintf(stderr, "Can't pop from empty stack.\n");
    return;
  }
  stack->height--;
  if (stack->height % stack->page_size == 0) {
    old_page = stack->top_page;
    stack->top_page = stack->top_page->below;
    free(old_page);
  }
}

void
stack_push_pointer(struct stack *stack, void *ptr)
{
  *(void**)stack_push(stack) = ptr;
}

void *
stack_pop_pointer(struct stack *stack)
{
  void *ptr;
  if (stack->height <= 0)
    return NULL;
  ptr = *((void **)stack->top_page->data 
    + stack->item_size * ((stack->height - 1) % stack->page_size));
  stack_pop(stack);
  return ptr;
}

void
stack_free(struct stack *stack)
{
  struct stack_page *old_page;
  while (stack->top_page) {
    old_page = stack->top_page;
    stack->top_page = stack->top_page->below;
    free(old_page);
  }
}
