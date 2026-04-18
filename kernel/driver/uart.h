#pragma once
#include <stdint.h>

void init_serial(void);
void write_serial(char c);
void serial_puts(const char *s);