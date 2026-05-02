#include <stdint.h>
#include "proc.h"

uint64_t sys_fork(void) {
    return fork();
}