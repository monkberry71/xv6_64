#include <stdint.h>
#define GOP_RED 0x00FF0000
#define GOP_GRN 0x0000FF00
#define GOP_BLU 0x000000FF

void gop_init(void);
void gop_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void gop_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);