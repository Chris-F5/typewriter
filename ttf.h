#include <stdint.h>

struct font_info {
  uint16_t units_per_em;
  int x_min;
  int y_min;
  int x_max;
  int y_max;
  uint16_t long_hor_metrics_count;
  uint16_t cmap[256];
  unsigned int char_widths[256];
};

int read_ttf(const char *ttf, long ttf_size, struct font_info *info);
