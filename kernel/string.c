#include <stdint.h>

void* memset(void* dst, int c, uint64_t n) {
    char *p = dst;
    while(n-- > 0) *p++ = c;
    return dst;
}

void *memcpy(void *dst, const void *src, uint64_t n) {
    char *d = dst, *s = src;
    while (n--) *d++ = *s++;
    return dst;
}
