#include "tw.h"

enum style_commands {
  STYLE_NONE,
  STYLE_SEQ,
  STYLE_TEXT,
  STYLE_CONTAINER,
  STYLE_SPACE,
  STYLE_BREAK,
  END_STYLE,
};

typedef void (*StyleFunction)(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info);

static void style_sequence(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info);
static void style_text(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info);
static void style_container(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info);
static void style_space(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info);
static void style(const int **command, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info);
static void print_atom_list(struct atom_list *list, int indent);

const int * const style_map[] = {
  [SYMBOL_ROOT] = (int []) {
    STYLE_CONTAINER, ORIENTATION_VERTICAL,
  },
  [SYMBOL_PARAGRAPH] = (int []) {
    STYLE_CONTAINER, ORIENTATION_HORIZONTAL,
  },
  [SYMBOL_WORD] = (int []) {
    STYLE_SEQ,
      STYLE_TEXT, 12,
      STYLE_SPACE, 12,
    END_STYLE,
  },
};

const StyleFunction style_functions[] = {
  [STYLE_NONE] = NULL,
  [STYLE_SEQ] = style_sequence,
  [STYLE_TEXT] = style_text,
  [STYLE_CONTAINER] = style_container,
  [STYLE_SPACE] = style_space,
};

static void
style_sequence(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info)
{
  while (**operands != END_STYLE)
    style(operands, sym, list_end, gizmo_stack, font_info);
  *operands += 1;
}

static void
style_text(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info)
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

  atom->font_size = (*operands)[0];
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

  *operands += 1;
}

static void
style_container(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info)
{
  const int *child_style_command;
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
  for (sym_child = sym->children; sym_child; sym_child = sym_child->next) {
    if (sym_child->type > sizeof(style_map) / sizeof(style_map[0])
        || style_map[sym_child->type] == NULL) {
      fprintf(stderr, "No style map entry for symbol '%d'.\n", sym_child->type);
      continue;
    }
    child_style_command = style_map[sym_child->type];
    style(&child_style_command, sym_child, &children_end,
        gizmo_stack, font_info);
  }

  *list_end = &(**list_end = gizmo_list)->next;
}

static void
style_space(const int **operands, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info)
{
  int font_size;
  struct gizmo_list *gizmo_list;
  struct graphic_gizmo *graphic;

  font_size = (*operands)[0];

  gizmo_list = stack_allocate(gizmo_stack, sizeof(struct gizmo_list));
  graphic = stack_allocate(gizmo_stack, sizeof(struct graphic_gizmo));

  graphic->gizmo_type = GIZMO_GRAPHIC;
  graphic->width = font_info->char_widths[' '] * font_size / 1000;
  graphic->height = font_size;
  graphic->text = NULL;

  gizmo_list->next = NULL;
  gizmo_list->gizmo = (struct gizmo *)graphic;

  *list_end = &(**list_end = gizmo_list)->next;

  *operands += 1;
}

static void
style(const int **command, const struct symbol *sym,
    struct gizmo_list ***list_end, struct stack *gizmo_stack,
    const struct font_info *font_info)
{
  int opcode;
  StyleFunction function;
  opcode = *(*command)++;
  if (opcode >= sizeof(style_functions) / sizeof(style_functions[0])) {
    fprintf(stderr, "Invalid style opcode '%d'.\n", opcode);
    return;
  }
  function = style_functions[opcode];
  if (function == NULL)
    return;
  function(command, sym, list_end, gizmo_stack, font_info);
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
  const int *root_style_command;
  struct gizmo_list **list_end;
  struct gizmo_list *list_start;
  if (sym->type > sizeof(style_map) / sizeof(style_map[0])
      || style_map[sym->type] == NULL) {
    fprintf(stderr, "No style map entry for root symbol '%d'.\n", sym->type);
    return NULL;
  }
  root_style_command = style_map[sym->type];
  list_end = &list_start;
  style(&root_style_command, sym, &list_end, gizmo_stack, font_info);
  if (list_start->gizmo->gizmo_type != GIZMO_CONTAINER) {
    fprintf(stderr, "Root gizmo must be container not '%d'.\n",
        list_start->gizmo->gizmo_type);
    return NULL;
  }
  return (struct container_gizmo *)list_start->gizmo;
}
