#pragma once
#include <stdint.h>

void init_serial(void);
void serial_putc(char c);
void serial_puts(const char *s);
void serial_hex(uint64_t v);