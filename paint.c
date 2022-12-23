#include "tw.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct text_state {
  int x, y;
};

static void
content_printf(struct content *content, const char *format, ...)
{
  va_list ap;
  int n;
  va_start(ap, format);
  n = vsnprintf(content->str + content->length,
      content->allocated - content->length, format, ap);
  if (content->length + n >= content->allocated) {
    content->allocated = content->length + n + 1024;
    content->str = xrealloc(content->str, content->allocated);
    vsprintf(content->str + content->length, format, ap);
  }
  content->length += n;
  va_end(ap);
}

static void
content_escaped_string(struct content *content, const char *str, int str_len)
{
  int max_str_length;
  char c;
  max_str_length = str_len * 2;
  if (content->length + max_str_length >= content->allocated) {
    content->allocated = content->length + max_str_length + 1024;
    content->str = xrealloc(content->str, content->allocated);
  }
  for (; str_len; str_len--) {
    c = *(str++);
    if (c == '\0')
      fprintf(stderr, "Content string can not contain null character.");
    if (c == '(' || c == ')' || c == '\\')
      content->str[content->length++] = '\\';
    content->str[content->length++] = c;
  }
  content->str[content->length] = '\0';
}

static void
content_put_char(struct content *content, char c)
{
  if (content->length + 1 >= content->allocated) {
    content->allocated = content->length + 1024;
    content->str = xrealloc(content->str, content->allocated);
  }
  content->str[content->length++] = c;
  content->str[content->length] = '\0';
}

void
content_init(struct content *content)
{
  content->length = 0;
  content->allocated = 8 * 1024;
  content->str = xmalloc(content->allocated);
}

void
content_free(struct content *content)
{
  free(content->str);
}

static void
paint_text(int x, int y, const struct layout_box *box, struct content *content,
    struct text_state *state)
{
  struct layout_box *child;
  if (box->str) {
    content_printf(content, "%d %d Td (", x - state->x, y - state->y);
    content_escaped_string(content, box->str, box->str_len);
    content_printf(content, ")Tj\n", x, y);
    state->x = x;
    state->y = y;
  }
  for (child = box->child_first; child; child = child->next_sibling)
    paint_text(x + child->x, y + box->height - child->y - child->height, child,
        content, state);
}

void
paint_content(const struct layout_box *box, struct content *content)
{
  struct text_state text_state;
  text_state.x = 0;
  text_state.y = 0;
  content_printf(content, "0 0 0 rg\n");
  content_printf(content, "BT /F1 12 Tf 0 Tw\n");
  paint_text(0, 0, box, content, &text_state);
  content_printf(content, "ET\n");
}
