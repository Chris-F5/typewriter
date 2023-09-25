/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

struct dbuffer {
  int size, allocated, increment;
  char *data;
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

/* ttf.c */
int read_ttf(FILE *file, struct font_info *info);
