#pragma once
#include <stdint.h>
#include "../gop.h"

#define MAX_COLS 256
#define MAX_ROWS 128


void font_draw_char(uint64_t x, uint64_t y, char c, uint64_t fg, uint64_t bg);

struct console {
    char buffer[MAX_ROWS][MAX_COLS];
    int cur_y, cur_x;

    int max_cols, max_rows;

    uint64_t fg, bg;

    struct gop_fb *gop_fb;
    uint64_t pixels_per_scanline;

    int font_w, font_h;
};

void console_putc(char c);
void console_init(void);
void cprintf(char *fmt, ...);