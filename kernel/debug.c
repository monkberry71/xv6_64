#include <stdint.h>
#include <stdarg.h>
#include "driver/uart.h"

void panic(char* str) {
    serial_puts(str);
    for(;;) {
        __asm__ volatile("hlt");
    }
}

static char digits[] = "0123456789abcdef";

static void print_uint(uint64_t x, int base, int sign) {
    char buf[32];

    if(sign && (int64_t)x < 0) {
        serial_putc('-');
        x = -(int64_t)x;
    }

    int i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);

    while(--i >= 0) {
        serial_putc(buf[i]);
    }
}

void serial_printf(char *fmt, ...) {
    va_list ap;
    if(fmt == 0) panic("null fmt");

    int c;
    va_start(ap, fmt);
    for(int i=0; (c = fmt[i] & 0xFF) != 0; i++) {
        if(c != '%') {
            // if %, just print bro
            serial_putc(c);
            continue;
        }
        
        // ok c is the one that come after %
        c = fmt[++i] & 0xFF;
        if(c==0) break;
        
        switch(c) {
            case 'd': {
                print_uint(va_arg(ap, int), 10, 1);
                break;
            }
            case 'x': {
                print_uint(va_arg(ap, uint64_t), 16, 0);
                break;
            }
            case 'p': {
                print_uint((uint64_t)va_arg(ap, void*), 16, 0);
                break;
            }
            case '%': {
                serial_putc('%');
                break;
            }
            case 's': {
                char *s;
                if((s = va_arg(ap, char*)) == 0) {
                    s = "[null]";
                }
                for(; *s; s++) serial_putc(*s);
                break;
            }
            default: {
                serial_putc('%');
                serial_putc(c);
                break;
            }
        }
    }
    va_end(ap);

}
