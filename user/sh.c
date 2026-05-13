#include <stdint.h>
#include "user.h"

int main(void) {
    const char *to_print = "sh test\n";
    const char *to_print2 = "wow!!\n";
    for(;;) {
        write(1, to_print, 8);
        write(1, to_print2, 6);
        for(volatile uint64_t i = 0; i < 60000000; i++);
    }
}