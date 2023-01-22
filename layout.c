#include "tw.h"

static void add_atoms(struct atom_list ***next, const struct atom_list *atoms,
    int x_offset, int y_offset, struct stack *stack);
static void fragment(int orientation, const struct gizmo_list *gizmos,
    int width, int height, struct gizmo_list ***list_end, struct stack *stack);

static void
add_atoms(struct atom_list ***end, const struct atom_list *atoms,
    int x_offset, int y_offset, struct stack *stack)
{
  for (; atoms; atoms = atoms->next) {
    **end = stack_allocate(stack, sizeof(struct atom_list));
    (**end)->x_offset = atoms->x_offset + x_offset;
    (**end)->y_offset = atoms->y_offset + y_offset;
    (**end)->next = NULL;
    (**end)->atom = atoms->atom;
    *end = &(**end)->next;
  }
}

static void
fragment(int orientation, const struct gizmo_list *gizmos, int width, int height,
    struct gizmo_list ***list_end, struct stack *stack)
{
  struct graphic_gizmo *graphic;
  struct graphic_gizmo *new_graphic;
  struct atom_list **new_graphic_next_atom;
  int is_vertical;
  int length, breadth, gizmo_length, gizmo_breadth, greatest_breadth;
  int progress;

  is_vertical = orientation == ORIENTATION_VERTICAL;
  length = is_vertical ? height : width;
  breadth = is_vertical ? width : height;
  progress = 0;

  new_graphic = stack_allocate(stack, sizeof(struct graphic_gizmo));

  new_graphic->gizmo_type = GIZMO_GRAPHIC;
  new_graphic->next = NULL;
  new_graphic->width = width;
  new_graphic->height = height;
  new_graphic->text = NULL;
  new_graphic_next_atom = &new_graphic->text;

  for (; gizmos; gizmos = gizmos->next) {
    if (gizmos->gizmo->gizmo_type == GIZMO_GRAPHIC) {
      graphic = (struct graphic_gizmo *)gizmos->gizmo;
      gizmo_length = is_vertical ? graphic->height : graphic->width;
      gizmo_breadth = is_vertical ? graphic->width : graphic->height;
      if (progress + gizmo_length > length) {
        fprintf(stderr, "Cant fit graphic gizmos in container.\n");
        break;
      }
      if (gizmo_breadth > greatest_breadth)
        greatest_breadth = gizmo_breadth;
      add_atoms(&new_graphic_next_atom, graphic->text,
          is_vertical ? 0 : progress, is_vertical ? progress : 0, stack);
      progress += gizmo_length;
    } else {
      fprintf(stderr, "Can't fragment gizmo type '%d'\n",
          gizmos->gizmo->gizmo_type);
      break;
    }
  }
  (**list_end) = stack_allocate(stack, sizeof(struct gizmo_list));
  (**list_end)->next = NULL;
  (**list_end)->gizmo = (struct gizmo *)new_graphic;
  *list_end = &(**list_end)->next;
}

void
layout(const struct container_gizmo *container, int width, int height,
    struct gizmo_list ***list_end, struct stack *stack)
{
  struct stack tmp_stack;
  const struct gizmo_list *list;
  struct gizmo_list *tmp_list, **tmp_list_end;
  tmp_list = NULL;
  tmp_list_end = &tmp_list;
  stack_init(&tmp_stack, 1024 * 8);
  for (list = container->gizmos; list; list = list->next) {
    if (list->gizmo->gizmo_type == GIZMO_CONTAINER) {
      layout((struct container_gizmo *)list->gizmo, width, height,
          &tmp_list_end, &tmp_stack);
    } else {
      *tmp_list_end = stack_allocate(&tmp_stack, sizeof(struct gizmo_list));
      **tmp_list_end = *list;
      (*tmp_list_end)->next = NULL;
      tmp_list_end = &(*tmp_list_end)->next;
    }
  }
  fragment(container->orientation, tmp_list, width, height, list_end, stack);

  stack_free(&tmp_stack, 0);
}
