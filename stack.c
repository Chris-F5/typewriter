#include "tw.h"

#include <stdlib.h>

void
stack_init(struct stack *stack, int page_size)
{
  stack->page_size = page_size;
  stack->height = 0;
  stack->top_page = NULL;
}

void *
stack_allocate(struct stack *stack, int size)
{
  int page_space;
  struct stack_page *new_page;
  void *ptr;
  if (stack->top_page) {
    page_space = stack->top_page->size + stack->top_page->height
      - stack->height;
    if (size <= page_space) {
      ptr = stack->top_page->data + stack->height - stack->top_page->height;
      stack->height += size;
      return ptr;
    }
  }

  page_space = size > stack->page_size ? size : stack->page_size;
  new_page = xmalloc(sizeof(struct stack_page) + page_space);
  new_page->size = page_space;
  new_page->height = stack->height;
  new_page->below = stack->top_page;
  stack->top_page = new_page;
  ptr = stack->top_page->data + stack->height - stack->top_page->height;

  stack->height += size;
  return ptr;
}

void
stack_free(struct stack *stack, int height)
{
  struct stack_page *old_page;
  while (stack->top_page && stack->top_page->height >= height) {
    old_page = stack->top_page;
    stack->top_page = stack->top_page->below;
    free(old_page);
  }
  stack->height = height;
}

void
stack_push_pointer(struct stack *stack, void *ptr)
{
  *(void **)stack_allocate(stack, sizeof(void *)) = ptr;
}

void *
stack_pop_pointer(struct stack *stack)
{
  void * ptr;
  if (stack->top_page == NULL || stack->height < sizeof(void *))
    return NULL;

  if (stack->height - sizeof(void *) < stack->top_page->height) {
    fprintf(stderr, "Can't pop pointer from stack. Top item too small.");
    return NULL;
  }

  ptr = *(void **)(stack->top_page->data + stack->height - sizeof(void *)
    - stack->top_page->height);
  stack_free(stack, stack->height - sizeof(void *));
  return ptr;
}
