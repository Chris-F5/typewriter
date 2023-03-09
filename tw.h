/*
 * include stdio.h
 */

struct dbuffer {
  int size, allocated, increment;
  char *data;
};

struct record {
  struct dbuffer string;
  int field_count;
  int fields_allocated;
  const char **fields;
};

struct stack {
  int page_size, height;
  struct stack_page {
    int size, height;
    struct stack_page *below;
    char data[];
  } *top_page;
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

struct pdf_page_list {
  int parent;
  int count, allocated;
  int *page_objs;
};

struct pdf_xref_table {
  int obj_count, allocated;
  long *obj_offsets;
};

/* error.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);

/* dbuffer.c */
void dbuffer_init(struct dbuffer *buf, int initial, int increment);
void dbuffer_putc(struct dbuffer *buf, char c);
void dbuffer_printf(struct dbuffer *buf, const char *format, ...);
void dbuffer_free(struct dbuffer *buf);

/* record.c */
void init_record(struct record *record);
void begin_field(struct record *record);
int parse_record(FILE *file, struct record *record);
void free_record(struct record *record);

/* stack.c */
void stack_init(struct stack *stack, int page_size);
void *stack_allocate(struct stack *stack, int size);
void stack_free(struct stack *stack, int height);
void stack_push_pointer(struct stack *stack, void *ptr);
void *stack_pop_pointer(struct stack *stack);

/* ttf.c */
int read_ttf(FILE *file, struct font_info *info);

/* typeface.c */
void open_typeface(FILE *typeface_file);

/* pdf.c */
void init_pdf_xref_table(struct pdf_xref_table *xref);
int allocate_pdf_obj(struct pdf_xref_table *xref);
void init_pdf_page_list(struct pdf_page_list *page_list);
void pdf_page_list_append(struct pdf_page_list *page_list, int page);
void pdf_write_header(FILE *file);
void pdf_start_indirect_obj(FILE *file, struct pdf_xref_table *xref, int obj);
void pdf_end_indirect_obj(FILE *file);
void pdf_write_file_stream(FILE *pdf_file, FILE *data_file);
void pdf_write_text_stream(FILE *file, const char *data, long size);
void pdf_write_int_array(FILE *file, const int *values, int count);
void pdf_write_resources(FILE *file, int font_widths, int font_descriptor,
    const char *font_name);
void pdf_write_font_descriptor(FILE *file, int font_file, const char *font_name,
    int flags, int italic_angle, int ascent, int descent, int cap_height,
    int stem_vertical, int min_x, int min_y, int max_x, int max_y);
void pdf_write_page(FILE *file, int parent, int content);
void pdf_write_page_list(FILE *file, const struct pdf_page_list *pages,
    int resources);
void pdf_write_catalog(FILE *file, int page_list);
void pdf_write_footer(FILE *file, struct pdf_xref_table *xref, int root_obj);

/* print_pages.c */
int print_pages(FILE *pages_file, FILE *font_file, FILE *pdf_file);
