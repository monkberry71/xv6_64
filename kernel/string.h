#pragma once
#include <stdint.h>
void* memset(void* dst, int c, uint64_t n);
void* memcpy(void *dst, const void *src, uint64_t n);
char* safe_strcpy(char *s, const char *t, uint64_t n);
int strncmp(const char *s, const char *t, uint64_t n);
char* strncpy(char *s, const char *t, int n);
int strlen(char *s);