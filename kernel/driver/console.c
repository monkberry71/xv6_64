#include <stdint.h>
#include "font8x16.h"
#include "../gop.h"
#include "console.h"
#include <stdarg.h>


void font_draw_char(uint64_t x, uint64_t y, char c, uint64_t fg, uint64_t bg) {
    if(c < FONT_FIRST || c > FONT_LAST) c = '?';
    const uint8_t *glyph = font8x16[c - FONT_FIRST];
    for(int row = 0; row < FONT_CHAR_H; row++) {
        uint8_t bits = glyph[row];
        for(int col = 0; col < FONT_CHAR_W; col++) {
            uint64_t color = (bits & (1 << (7 - col))) ? fg : bg;
            gop_draw_pixel(x + col, y + row, color);
        }
    }
}

struct console g_console;

void console_redraw_all() {
    gop_draw_rect(0,0, 
        g_console.gop_fb->width,
        g_console.gop_fb->height,
        g_console.bg
    );

    for(int y=0; y < g_console.max_rows; y++) {
        for(int x=0; x < g_console.max_cols; x++) {
            char c = g_console.buffer[y][x];
            if (c != ' ' && c != 0) {
                font_draw_char(x*g_console.font_w,y*g_console.font_h, c, g_console.fg, g_console.bg);
            }
        }
    }
}

void console_scroll() {
    for(int y=0; y < g_console.max_rows - 1; y++) {
        for(int x=0; x < g_console.max_cols; x++) {
            g_console.buffer[y][x] = g_console.buffer[y+1][x];
        }
    }

    for(int x=0; x< g_console.max_cols; x++){
        g_console.buffer[g_console.max_rows- 1][x] = ' ';
    }

    console_redraw_all();
}

void console_putc(char c) {
    if(c == '\n') {
        g_console.cur_x = 0;
        g_console.cur_y++;
    } else if (c == '\r') {
        g_console.cur_x = 0;
    } else {
        g_console.buffer[g_console.cur_y][g_console.cur_x] = c;
        font_draw_char(g_console.cur_x*g_console.font_w,g_console.cur_y*g_console.font_h, c, g_console.fg, g_console.bg);
        g_console.cur_x++;
    }
    
    // next line
    if(g_console.cur_x >= g_console.max_cols) {
        g_console.cur_x = 0;
        g_console.cur_y++;
    }
    
    if(g_console.cur_y >= g_console.max_rows) {
        console_scroll();
        g_console.cur_y = g_console.max_rows - 1;
    }
}

static char digits[] = "0123456789abcdef";
static void print_int(int xx, int base, int sign) {
    char buf[16];
    
    unsigned x;
    if(sign && (sign = (xx < 0))) {
        x = -xx;
    } else {
        x = xx;
    }
    
    if(sign) {
        console_putc('-');
    }
    
    int i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);
    
    while(--i >= 0) {
        console_putc(buf[i]);
    }
}

void cprintf(char *fmt, ...) {
    va_list ap;
    if(fmt == 0) return;
    
    int c;
    va_start(ap, fmt);
    for(int i=0; (c = fmt[i] & 0xFF) != 0; i++) {
        if(c != '%') {
            // if %, just print bro
            console_putc(c);
            continue;
        }
        
        // ok c is the one that come after %
        c = fmt[++i] & 0xFF;
        if(c==0) break;
        
        switch(c) {
            case 'd': {
                print_int(va_arg(ap, int), 10, 1);
                break;
            }
            case 'x': {
                print_int(va_arg(ap, int), 16, 0);
                break;
            }
            case '%': {
                console_putc('%');
                break;
            }
            case 's': {
                char *s;
                if((s = va_arg(ap, char*)) == 0) {
                    s = "[null]";
                }
                for(; *s; s++) console_putc(*s);
                break;
            }
            default: {
                console_putc('%');
                console_putc(c);
                break;
            }
        }
    }
    va_end(ap);
}

void console_init(void) {
    
    g_console.gop_fb = &gop_fb;

    g_console.font_w = 8;
    g_console.font_h = 16;

    g_console.max_cols = g_console.gop_fb->width / g_console.font_w;
    g_console.max_rows = g_console.gop_fb->height / g_console.font_h;

    g_console.cur_y = 0;
    g_console.cur_x = 0;

    g_console.fg = GOP_WHI;
    g_console.bg = GOP_BLK;

    g_console.pixels_per_scanline = g_console.gop_fb->pitch / (g_console.gop_fb->bpp / 8);
    // pitch means byte per row
    
}
