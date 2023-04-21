/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
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

struct jpeg_info {
  int width, height;
  unsigned char components;
};

struct pdf_resources {
  struct record fonts_used;
  struct record images;
};

struct pdf_page_list {
  int page_count, pages_allocated;
  int *page_objs;
};

struct pdf_xref_table {
  int obj_count, allocated;
  long *obj_offsets;
};

/* utils.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
int is_font_name_valid(const char *font_name);
int str_to_int(const char *str, int *n);

/* dbuffer.c */
void dbuffer_init(struct dbuffer *buf, int initial, int increment);
void dbuffer_putc(struct dbuffer *buf, char c);
void dbuffer_printf(struct dbuffer *buf, const char *format, ...);
void dbuffer_free(struct dbuffer *buf);

/* record.c */
void init_record(struct record *record);
void begin_field(struct record *record);
int parse_record(FILE *file, struct record *record);
int find_field(const struct record *record, const char *field_str);
void free_record(struct record *record);

/* ttf.c */
int read_ttf(FILE *file, struct font_info *info);

/* jpeg.c */
int read_jpeg(FILE *file, struct jpeg_info *info);

/* pdf.c */
void pdf_write_header(FILE *file);
void pdf_start_indirect_obj(FILE *file, struct pdf_xref_table *xref, int obj);
void pdf_end_indirect_obj(FILE *file);
void pdf_write_file_stream(FILE *pdf_file, FILE *data_file);
void pdf_write_text_stream(FILE *file, const char *data, long size);
void pdf_write_int_array(FILE *file, const int *values, int count);
void pdf_write_font_descriptor(FILE *file, int font_file, const char *font_name,
    int italic_angle, int ascent, int descent, int cap_height,
    int stem_vertical, int min_x, int min_y, int max_x, int max_y);
void pdf_write_page(FILE *file, int parent, int content);
void pdf_write_font(FILE *file, const char *font_name, int font_descriptor,
    int font_widths);
void pdf_write_pages(FILE *file, int resources, int page_count,
    const int *page_objs);
void pdf_write_catalog(FILE *file, int page_list);
void init_pdf_xref_table(struct pdf_xref_table *xref);
int allocate_pdf_obj(struct pdf_xref_table *xref);
void pdf_add_footer(FILE *file, const struct pdf_xref_table *xref,
    int root_obj);
void free_pdf_xref_table(struct pdf_xref_table *xref);
void init_pdf_resources(struct pdf_resources *resources);
void include_font_resource(struct pdf_resources *resources, const char *font);
int include_image_resource(struct pdf_resources *resources, const char *fname);
void pdf_add_resources(FILE *pdf_file, FILE *typeface_file, int resources_obj,
    const struct pdf_resources *resources, struct pdf_xref_table *xref);
void free_pdf_resources(struct pdf_resources *resources);
void init_pdf_page_list(struct pdf_page_list *page_list);
void add_pdf_page(struct pdf_page_list *page_list, int page);
void free_pdf_page_list(struct pdf_page_list *page_list);

/* print_pages.c */
int print_pages(FILE *pages_file, FILE *font_file, FILE *pdf_file);
