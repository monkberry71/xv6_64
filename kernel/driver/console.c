#include <stdint.h>
#include "font8x16.h"
#include "../gop.h"
#include "console.h"
#include <stdarg.h>
#include "../params.h"
#include "../file.h"
#include "../debug.h"
#include "../fs.h"
#include "../proc.h"
#include "kbd.h"


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

struct console cons;

void console_redraw_all() {
    gop_draw_rect(0,0, 
        cons.gop_fb->width,
        cons.gop_fb->height,
        cons.bg
    );

    for(int y=0; y < cons.max_rows; y++) {
        for(int x=0; x < cons.max_cols; x++) {
            char c = cons.buffer[y][x];
            if (c != ' ' && c != 0) {
                font_draw_char(x*cons.font_w,y*cons.font_h, c, cons.fg, cons.bg);
            }
        }
    }
}

void console_scroll() {
    for(int y=0; y < cons.max_rows - 1; y++) {
        for(int x=0; x < cons.max_cols; x++) {
            cons.buffer[y][x] = cons.buffer[y+1][x];
        }
    }

    for(int x=0; x< cons.max_cols; x++){
        cons.buffer[cons.max_rows- 1][x] = ' ';
    }

    console_redraw_all();
}

void console_putc(char c) {
    if(c == '\n') {
        cons.cur_x = 0;
        cons.cur_y++;
    } else if (c == '\r') {
        cons.cur_x = 0;
    } else if (c == '\b') {
        if(cons.cur_x == 0) return;
        cons.cur_x--;
        cons.buffer[cons.cur_y][cons.cur_x] = ' ';
        font_draw_char(cons.cur_x*cons.font_w,cons.cur_y*cons.font_h, ' ', cons.fg, cons.bg);
    } else {
        cons.buffer[cons.cur_y][cons.cur_x] = c;
        font_draw_char(cons.cur_x*cons.font_w,cons.cur_y*cons.font_h, c, cons.fg, cons.bg);
        cons.cur_x++;
    }
    
    // next line
    if(cons.cur_x >= cons.max_cols) {
        cons.cur_x = 0;
        cons.cur_y++;
    }
    
    if(cons.cur_y >= cons.max_rows) {
        console_scroll();
        cons.cur_y = cons.max_rows - 1;
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
    if(fmt == 0) panic("null fmt");

    if(cons.locking) acquire(&cons.lk);
    
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
    if(cons.locking) release(&cons.lk);
    va_end(ap);

}

#define INPUT_BUF 128
struct {
    char buf[INPUT_BUF];
    uint32_t r;  // Read index
    uint32_t w;  // Write index
    uint32_t e;  // Edit index
} input;

void console_intr(void) {
    acquire(&cons.lk);
    int c;
    while((c = kbd_getc()) >= 0) {
        switch(c) {
            case C('U'): { // delete the whole line buffer
                while(input.e != input.w && input.buf[(input.e-1) % INPUT_BUF] != '\n') {
                    input.e--;
                    console_putc('\b');
                }
                break;
            }
            case C('H'): {
                if(input.e != input.w) {
                    input.e--;
                    console_putc('\b');
                }
                break;
            }
            default: {
                if(c != 0 && input.e - input.r < INPUT_BUF) {
                    c = (c == '\r') ? '\n' : c; // change /r to /n, both are enters
                    input.buf[input.e++ % INPUT_BUF] = c;
                    console_putc(c);
                    if(c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
                        input.w = input.e; // update input.w to input.e
                        wakeup(&input.r);
                    }
                }
            }

        }
    }
    release(&cons.lk);
}

int64_t console_write(struct inode* ip, uint8_t *buf, uint64_t n) {
    iunlock(ip);
    acquire(&cons.lk);
    int64_t i;
    for(i=0; i < n; i++) {
        console_putc(buf[i] & 0xFF);
    }
    release(&cons.lk);
    ilock(ip);

    return i;
}

int64_t console_read(struct inode* ip, uint8_t *dst, uint64_t n) {
    iunlock(ip);
    int target = n; // original requesteed amount, n becomes remaining amount
    acquire(&cons.lk);
    while(n > 0) {
        while(input.r == input.w) {
            // condition variable, wait until input.w is updated to input.e 
            if(myproc()->killed) {
                release(&cons.lk);
                ilock(ip);
                return -1;
            }
            sleep(&input.r, &cons.lk);
        }


        int c = input.buf[input.r++ % INPUT_BUF];
        if(c == C('D')) {
            if(n < target) { // have we copied at least one byte?
                // save ctrl+d for next time, to make sure caller gets a 0 byte result
                input.r--;
            }
            break;
        }
        *dst++ = c;
        n--;
        if(c == '\n') break;
    }
    release(&cons.lk);
    ilock(ip);
    return target - n; // actually read byte
}

extern struct dev_sw devs[NDEV];
void console_init(void) {

    init_lock(&cons.lk, "console");
    
    cons.gop_fb = &gop_fb;

    cons.font_w = 8;
    cons.font_h = 16;

    cons.max_cols = cons.gop_fb->width / cons.font_w;
    cons.max_rows = cons.gop_fb->height / cons.font_h;

    cons.cur_y = 0;
    cons.cur_x = 0;

    cons.fg = GOP_WHI;
    cons.bg = GOP_BLK;

    cons.pixels_per_scanline = cons.gop_fb->pitch / (cons.gop_fb->bpp / 8);
    // pitch means byte per row
    
    devs[CONSOLE_DEVNUM].write = console_write;
    devs[CONSOLE_DEVNUM].read = console_read;
    cons.locking = 1;

}
