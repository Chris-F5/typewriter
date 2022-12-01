#include <stdint.h>

struct font_info {
  uint16_t units_per_em;
  uint16_t glyph_count;
  uint16_t long_hor_metrics_count;
  uint16_t cmap[256];
  uint16_t char_widths[256];
};

int read_ttf(const char *ttf, long ttf_size, struct font_info *info);
