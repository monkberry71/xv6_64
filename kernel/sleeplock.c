#include <stdint.h>
#include "sleeplock.h"
#include "spinlock.h"
#include "proc.h"


void init_sleep_lock(struct sleep_lock *lk, char *name) {
    init_lock(&lk->lk, "sleep lock");
    lk->name = name;
    lk->locked = 0;
    lk->pid = 0;
}

void acquire_sleep(struct sleep_lock *lk) {
    acquire(&lk->lk);
    // protect the condition!!!
    while (lk->locked) {
        sleep(lk, &lk->lk);
    }
    lk->locked = 1;
    lk->pid = myproc()->pid;
    release(&lk->lk);
}

void release_sleep(struct sleep_lock *lk) {
    acquire(&lk->lk);
    lk->locked = 0;
    lk->pid = 0;
    wakeup(lk);
    release(&lk->lk);
}

int holding_sleep(struct sleep_lock *lk) {
    acquire(&lk->lk);
    int r = lk->locked && (lk->pid == myproc()->pid);
    release(&lk->lk);
    return r;
}