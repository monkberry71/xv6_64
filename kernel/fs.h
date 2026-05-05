#pragma once
#include <stdint.h>
#include "sleeplock.h"

#define BSIZE 4096
#define DISK_SIZE (16 * 1024 * 1024)
#define NBLOCKS (DISK_SIZE / BSIZE)

#define B_VALID 0x2
#define B_DIRTY 0x4

#define DEV_RAMDISK 1

struct buf {
    uint64_t flags;
    uint64_t dev;
    uint64_t block_no;
    struct sleep_lock lk;
    uint32_t ref_count;
    struct buf *prev, *next, *qnext;
    uint8_t data[BSIZE];
};
