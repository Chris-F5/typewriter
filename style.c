#include "tw.h"

static void style_word(const struct symbol *sym,
    const struct font_info *font_info, struct gizmo_list ***list_end,
    struct stack *gizmo_stack);
static void style(const struct symbol *sym, const struct font_info *font_info,
    struct gizmo_list ***list_end, struct stack *gizmo_stack);
static void print_atom_list(struct atom_list *list, int indent);

static void
style_word(const struct symbol *sym, const struct font_info *font_info,
    struct gizmo_list ***list_end, struct stack *gizmo_stack)
{
  int i;
  struct gizmo_list *gizmo_list;
  struct graphic_gizmo *graphic;
  struct atom_list *atom_list;
  struct text_atom *atom;

  gizmo_list = stack_allocate(gizmo_stack, sizeof(struct gizmo_list));
  graphic = stack_allocate(gizmo_stack, sizeof(struct graphic_gizmo));
  atom_list = stack_allocate(gizmo_stack, sizeof(struct atom_list));
  atom = stack_allocate(gizmo_stack, sizeof(struct text_atom));

  atom->font_size = 12;
  atom->str_len = sym->str_len;
  atom->str = sym->str;

  atom_list->x_offset = 0;
  atom_list->y_offset = 0;
  atom_list->next = NULL;
  atom_list->atom = atom;

  graphic->gizmo_type = GIZMO_GRAPHIC;
  graphic->width = 0;
  for (i = 0; i < atom->str_len; i++)
    graphic->width += font_info->char_widths[atom->str[i]];
  graphic->width = graphic->width * atom->font_size / 1000;
  graphic->height = atom->font_size;
  graphic->text = atom_list;

  gizmo_list->next = NULL;
  gizmo_list->gizmo = (struct gizmo *)graphic;

  *list_end = &(**list_end = gizmo_list)->next;
}

static void
style_paragraph(const struct symbol *sym, const struct font_info *font_info,
    struct gizmo_list ***list_end, struct stack *gizmo_stack)
{
  struct container_gizmo *container;
  struct gizmo_list **children_end, *gizmo_list;
  struct symbol *sym_child;
  gizmo_list  = stack_allocate(gizmo_stack, sizeof(struct gizmo_list));
  container = stack_allocate(gizmo_stack, sizeof(struct container_gizmo));
  container->gizmo_type = GIZMO_CONTAINER;
  container->orientation = ORIENTATION_HORIZONTAL;
  container->gizmos = NULL;
  gizmo_list->next = NULL;
  gizmo_list->gizmo = (struct gizmo *)container;
  children_end = &container->gizmos;
  for (sym_child = sym->children; sym_child; sym_child = sym_child->next)
    style(sym_child, font_info, &children_end, gizmo_stack);

  *list_end = &(**list_end = gizmo_list)->next;
}

static void
style(const struct symbol *sym, const struct font_info *font_info,
    struct gizmo_list ***list_end, struct stack *gizmo_stack)
{
  switch (sym->type) {
  case SYMBOL_WORD:
    style_word(sym, font_info, list_end, gizmo_stack);
    break;
  case SYMBOL_PARAGRAPH:
    style_paragraph(sym, font_info, list_end, gizmo_stack);
    break;
  default:
    printf("Can't style symbol type '%d'\n", sym->type);
  }
}

static void
print_atom_list(struct atom_list *list, int indent)
{
  int i;
  for (i = 0; i < indent * 2; i++) putchar(' ');
  printf("atom list:\n");
  for (; list; list = list->next) {
    for (i = 0; i < indent * 2 + 2; i++) putchar(' ');
    printf("%d %d\n", list->x_offset, list->y_offset);
  }
}

void
print_gizmo(struct gizmo *gizmo, int indent)
{
  int i;
  struct gizmo_list *child;
  if (gizmo->gizmo_type == GIZMO_CONTAINER) {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    printf("container:\n");
    for (
        child = ((struct container_gizmo *)gizmo)->gizmos;
        child;
        child = child->next)
      print_gizmo(child->gizmo, indent + 1);
  } else if (gizmo->gizmo_type == GIZMO_GRAPHIC) {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    printf("graphic:\n");
    print_atom_list(((struct graphic_gizmo *)gizmo)->text, indent + 1);
  } else {
    for (i = 0; i < indent * 2; i++) putchar(' ');
    printf("%d\n", gizmo->gizmo_type);
  }
}

struct container_gizmo *
style_document(const struct symbol *sym, const struct font_info *font_info,
    struct stack *gizmo_stack)
{
  struct container_gizmo *root_gizmo;
  struct symbol *child_sym;
  struct gizmo_list **list_end;
  root_gizmo = stack_allocate(gizmo_stack, sizeof(struct container_gizmo));
  root_gizmo->gizmo_type = GIZMO_CONTAINER;
  root_gizmo->orientation = ORIENTATION_VERTICAL;
  root_gizmo->gizmos = NULL;
  list_end = &root_gizmo->gizmos;
  for (child_sym = sym->children; child_sym; child_sym = child_sym->next) {
    style(child_sym, font_info, &list_end, gizmo_stack);
  }
  return root_gizmo;
}
