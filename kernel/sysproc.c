#include <stdint.h>
#include "proc.h"

uint64_t sys_fork(void) {
    return fork();
}

uint64_t sys_wait(void) {
    return wait();
}

uint64_t sys_exit(void) {
    exit();
    return 0; // no reach
}