#pragma once
#include <stdint.h>
#include "spinlock.h"

struct sleep_lock {
    uint32_t locked; 
    struct spin_lock lk; // spinlock protecting this lock

    char *name; // name of this lock
    uint64_t pid; // process that holds this lock
};

void init_sleep_lock(struct sleep_lock *lk, char *name);
void acquire_sleep(struct sleep_lock *lk);
void release_sleep(struct sleep_lock *lk);
int holding_sleep(struct sleep_lock *lk);