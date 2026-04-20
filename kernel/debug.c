#include <stdint.h>
#include "driver/uart.h"

void panic(char* str) {
    serial_puts(str);
    for(;;) {
        __asm__ volatile("hlt");
    }
}