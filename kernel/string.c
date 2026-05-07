#include <stdint.h>

void* memset(void* dst, int c, uint64_t n) {
    char *p = dst;
    while(n-- > 0) *p++ = c;
    return dst;
}

void *memcpy(void *dst, void *src, uint64_t n) {
    char *d = dst, *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

char* safe_strcpy(char *s, const char *t, uint64_t n) {
    char *os = s;
    if(n <= 0) return os;
    while(--n > 0 && (*s++ = *t++) != 0);
    *s = 0;
    return os;
}

int strncmp(const char *s, const char *t, uint64_t n) {
    while(n>0 && *s && *s == *t) {
        n--;
        s++;
        t++;
    }
    if(n == 0) return 0;
    return (uint8_t)*s - (uint8_t)*t;
}

char* strncpy(char *s, const char *t, int n) {
    char* os = s;
    while(n-->0 && (*s++ = *t++) != 0);
    while(n-->0) *s++ = 0;
    return os;
}