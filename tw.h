#include <errno.h>
#include <stdio.h>

#define OR ? (errno = 0) : (errno = errno ? errno : -1); if (errno)

enum symbol_type {
  SYMBOL_NONE = 0,
  SYMBOL_DOCUMENT,
  SYMBOL_PARAGRAPH,
  SYMBOL_REGULAR_WORD,
  SYMBOL_BOLD_WORD,
};

enum layout_type {
  LAYOUT_VERTICAL,
  LAYOUT_HORIZONTAL,
  LAYOUT_WORD,
};

struct stack {
  int item_size, page_size, height;
  struct stack_page {
    struct stack_page *below;
    char data[];
  } *top_page;
};

struct symbol {
  int type;
  int str_len;
  const char *str;
  struct symbol *child_first;
  struct symbol *child_last;
  struct symbol *next_sibling;
};

struct style {
  int display_type;
  int padding_top, padding_bottom, padding_left, padding_right;
  int font_size;
};

struct style_node {
  const struct style *style;
  int str_len;
  const char *str;
  struct style_node *child_first;
  struct style_node *child_last;
  struct style_node *next_sibling;
};

struct layout_box {
  int x, y;
  int width, height;
  int str_len;
  const char *str;
  struct layout_box *child_first;
  struct layout_box *child_last;
  struct layout_box *next_sibling;
};

struct content {
  int allocated, length;
  char *str;
};

struct font_info {
  int units_per_em;
  int x_min;
  int y_min;
  int x_max;
  int y_max;
  int long_hor_metrics_count;
  int cmap[256];
  int char_widths[256];
};

struct pdf_ctx {
  FILE *file;
  int obj_count;
  int obj_allocated;
  long *obj_offsets;
};

/* error.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);

/* stack.c */
void stack_init(struct stack *stack, int page_size, int item_size);
void *stack_push(struct stack *stack);
void stack_pop(struct stack *stack);
void stack_push_pointer(struct stack *stack, void *ptr);
void *stack_pop_pointer(struct stack *stack);
void stack_free(struct stack *stack);

/* parse.c */
void print_symbol_tree(struct symbol* sym, int indent);
struct symbol *parse_document(const char *document, struct stack *sym_stack);

/* style.c */
void print_style_tree(struct style_node* style_node, int indent);
struct style_node *create_style_tree(const struct symbol *root_sym,
    struct stack *style_node_stack);

/* layout.c */
void print_layout_tree(const struct layout_box *box, int indent);
struct layout_box *layout_pages(struct style_node *root_style_node,
    struct stack *layout_stack, const struct font_info *font);

/* paint.c */
void content_init(struct content *content);
void content_free(struct content *content);
void paint_content(const struct layout_box *layout, struct content *content);

/* ttf.c */
int read_ttf(const char *ttf, long ttf_size, struct font_info *info);

/* pdf.c */
void pdf_init(struct pdf_ctx *pdf, FILE *file);
int pdf_allocate_obj(struct pdf_ctx *pdf);
void pdf_add_content(struct pdf_ctx *pdf, int obj,
    const struct content *content);
void pdf_add_true_type_program(struct pdf_ctx *pdf, int obj, const char *ttf,
    long ttf_size);
void pdf_add_int_array(struct pdf_ctx *pdf, int obj, const int *valeus,
    int count);
void pdf_add_resources(struct pdf_ctx *pdf, int obj, int font_widths,
    int font_descriptor, const char *font_name);
void pdf_add_font_descriptor(struct pdf_ctx *pdf, int obj, int font_file,
    const char *font_name, int flags, int italic_angle, int ascent, int descent,
    int cap_height, int stem_vertical, int min_x, int min_y, int max_x,
    int max_y);
void pdf_add_page(struct pdf_ctx *pdf, int obj, int parent, int resources,
    int content);
void pdf_add_page_list(struct pdf_ctx *pdf, int obj, const int *pages,
    int page_count);
void pdf_add_catalog(struct pdf_ctx *pdf, int obj, int page_list);
void pdf_end(struct pdf_ctx *pdf, int root_obj);
