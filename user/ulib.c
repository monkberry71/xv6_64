#include <stdint.h>
#include <stdarg.h>
#include "user.h"

uint64_t strlen(const char *s) {
    int n;
    for(n = 0; s[n]; n++);
    return n;
}

static void putc(int fd, char c) {
    write(fd, &c, 1);
}

static char digits[] = "0123456789abcdef";
static void print_int(int fd, int xx, int base, int sign) {
    char buf[16];
    
    unsigned x;
    if(sign && (sign = (xx < 0))) {
        x = -xx;
    } else {
        x = xx;
    }
    
    
    int i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);
    
    if(sign) {
        buf[i++] = '-';
    }
    while(--i >= 0) {
        putc(fd, buf[i]);
    }
}

void printf(int fd, const char *fmt, ...) {
    va_list ap;
    if(fmt == 0) {
        return;
    }
    
    int c;
    va_start(ap, fmt);
    for(int i=0; (c = fmt[i] & 0xFF) != 0; i++) {
        if(c != '%') {
            // if %, just print bro
            // console_putc(c)
            putc(fd, c);
            continue;
        }
        
        // ok c is the one that come after %
        c = fmt[++i] & 0xFF;
        if(c==0) break;
        
        switch(c) {
            case 'd': {
                print_int(fd, va_arg(ap, int), 10, 1);
                break;
            }
            case 'x': {
                print_int(fd, va_arg(ap, int), 16, 0);
                break;
            }
            case '%': {
                putc(fd, c);
                break;
            }
            case 's': {
                char *s;
                if((s = va_arg(ap, char*)) == 0) {
                    s = "[null]";
                }
                for(; *s; s++) putc(fd, *s);
                break;
            }
            default: {
                // console_putc('%');
                // console_putc(c);
                putc(fd, '%');
                putc(fd, c);
                break;
            }
        }
    }
    va_end(ap);

}