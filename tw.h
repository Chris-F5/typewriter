#include <errno.h>
#include <stdio.h>

#define OR ? (errno = 0) : (errno = errno ? errno : -1); if (errno)

enum parser_opcodes {
  /* primitives */
  PARSE_CHAR,
  PARSE_CHAR_RANGE,
  PARSE_GRAMMAR,
  /* symbol constructors */
  PARSE_SYMBOL,
  PARSE_SYMBOL_STRING,
  /* patterns */
  PARSE_CHOICE,
  PARSE_SEQ,
  PARSE_ANY,
  PARSE_SOME,
  PARSE_OPTIONAL,
  /* miscellaneous */
  END_PARSE = -256
};

enum symbol_type {
  SYMBOL_ROOT,
  SYMBOL_PARAGRAPH,
  SYMBOL_WORD,
};

enum gizmo_type {
  GIZMO_GRAPHIC,
  GIZMO_BREAK,
  GIZMO_CONTAINER,
};

enum container_orientation {
  ORIENTATION_VERTICAL,
  ORIENTATION_HORIZONTAL,
};

struct bytes {
  int count, allocated, increment;
  char *data;
};

struct stack {
  int page_size, height;
  struct stack_page {
    int size, height;
    struct stack_page *below;
    char data[];
  } *top_page;
};

struct symbol {
  int type;
  int str_len;
  const char *str;
  struct symbol *children;
  struct symbol *next;
};

struct gizmo_list {
  struct gizmo_list *next;
  struct gizmo *gizmo;
};

struct gizmo {
  int gizmo_type;
  char _[];
};

struct container_gizmo {
  int gizmo_type;
  int orientation;
  struct gizmo_list *gizmos;
};

struct break_gizmo {
  int gizmo_type;
  struct gizmo *pre_break;
  struct gizmo *post_break;
  struct gizmo *no_break;
};

struct graphic_gizmo {
  int gizmo_type;
  int width, height;
  struct atom_list *text;
};

struct atom_list {
  int x_offset, y_offset;
  struct atom_list *next;
  const void *atom;
};

struct text_atom {
  int font_size;
  int str_len;
  const char *str;
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

/* bytes.c */
void bytes_init(struct bytes *bytes, int initial, int increment);
void bytes_printf(struct bytes *bytes, const char *format, ...);
void bytes_free(struct bytes *bytes);

/* stack.c */
void stack_init(struct stack *stack, int page_size);
void *stack_allocate(struct stack *stack, int size);
void stack_free(struct stack *stack, int height);
void stack_push_pointer(struct stack *stack, void *ptr);
void *stack_pop_pointer(struct stack *stack);

/* parse.c */
void print_symbol_tree(struct symbol* sym, int indent);
struct symbol *parse_document(const char *document, struct stack *sym_stack);

/* style.c */
void print_gizmo(struct gizmo *gizmo, int indent);
struct container_gizmo *style_document(const struct symbol *sym,
    const struct font_info *font_info, struct stack *gizmo_stack);

/* layout.c */
void layout(const struct container_gizmo *container, int width, int height,
    struct gizmo_list ***list_end, struct stack *stack);

/* paint.c */
void paint_graphic(const struct graphic_gizmo *graphic, struct bytes *content,
    struct font_info *font_info);

/* ttf.c */
int read_ttf(const char *ttf, long ttf_size, struct font_info *info);

/* pdf.c */
void pdf_init(struct pdf_ctx *pdf, FILE *file);
int pdf_allocate_obj(struct pdf_ctx *pdf);
void pdf_add_true_type_program(struct pdf_ctx *pdf, int obj, const char *ttf,
    long ttf_size);
void pdf_add_stream(struct pdf_ctx *pdf, int obj, const char *bytes,
    long bytes_count);
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
