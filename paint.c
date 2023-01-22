#include "tw.h"

struct text_state {
  int line_x, line_y;
  int font_size;
};

static void write_pdf_string(struct bytes *bytes, const char *str, int str_len);
static void paint_text(const struct atom_list *list, struct bytes *content,
    struct font_info *font_info);

static void
write_pdf_string(struct bytes *bytes, const char *str, int str_len)
{
  int i;
  bytes_printf(bytes, "(");
  for (i = 0; i < str_len ; i++) {
    if (str[i] == '(' || str[i] == ')' || str[i] == '\\')
      bytes_printf(bytes, "\\");
    bytes_printf(bytes, "%c", str[i]);
  }
  bytes_printf(bytes, ")");
}

static void
paint_text(const struct atom_list *list, struct bytes *content,
    struct font_info *font_info)
{
  struct text_state state;
  struct text_atom *text;
  int text_x, text_y;
  state.line_x = 0;
  state.line_y = 0;
  state.font_size = -1;
  bytes_printf(content, "BT\n");
  for (; list; list = list->next) {
    text = (struct text_atom *)list->atom;
    text_x = list->x_offset;
    text_y = 842 - list->y_offset - text->font_size;

    bytes_printf(content, "%d %d Td\n", text_x - state.line_x,
        text_y - state.line_y);
    state.line_x = text_x;
    state.line_y = text_y;

    bytes_printf(content, "/F1 %d Tf\n", text->font_size);
    write_pdf_string(content, text->str, text->str_len);
    bytes_printf(content, " Tj\n", text->font_size);
  }
  bytes_printf(content, "ET\n");
}

void
paint_graphic(const struct graphic_gizmo *graphic, struct bytes *content,
    struct font_info *font_info)
{
  bytes_printf(content, "0 0 0 rg\n");
  paint_text(graphic->text, content, font_info);
}
