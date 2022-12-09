#include <errno.h>
#include <stdio.h>

#define OR ? (errno = 0) : (errno = errno ? errno : -1); if (errno)

struct content_line {
  int font_size;
  int word_spacing;
  char *text;
};

struct content_section {
  int width;
  int line_spacing;
  int line_count;
  int line_allocated;
  struct content_line *lines;
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

/* content.c */
void content_section_init(struct content_section *section, int width);
void content_section_add_line(struct content_section *section,
    struct content_line line);
void content_section_draw(struct content_section *section, FILE *stream, int x,
    int y);
void content_section_destroy(struct content_section *section);

/* error.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);

/* pdf.c */
void pdf_init(struct pdf_ctx *pdf, FILE *file);
int pdf_allocate_obj(struct pdf_ctx *pdf);
void pdf_add_stream(struct pdf_ctx *pdf, int obj, FILE *stream);
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

/* ttf.c */
int read_ttf(const char *ttf, long ttf_size, struct font_info *info);
