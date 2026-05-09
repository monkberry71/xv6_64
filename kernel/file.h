#pragma once
#include <stdint.h>
#include "sleeplock.h"

#define NDIRECT 13
#define NINDIRECT (BSIZE / sizeof(uint64_t))
#define MAXFILEBLK (NDIRECT + NINDIRECT)

struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE } type;
    int ref;
    char readable;
    char writable;
    // struct pipe
    struct inode *ip;
    uint64_t off;
};

struct inode {
    uint64_t dev;
    uint64_t inum;
    uint64_t ref;
    struct sleep_lock lock;
    int valid;

    short type;
    short major;
    short minor;
    short nlink;
    uint64_t size;
    uint64_t addrs[NDIRECT+1];
};

struct dev_sw {
    int (*read)(struct inode*, uint8_t*, uint64_t);
    int (*write)(struct inode*, uint8_t*, uint64_t);
};

extern struct dev_sw devs[];

#define CONSOLE 1