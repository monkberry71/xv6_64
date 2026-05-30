#pragma once
#include <stdint.h>
#include "sleeplock.h"
#include "stat.h"

#define NDIRECT 13
#define NINDIRECT (BSIZE / sizeof(uint64_t))
#define MAXFILEBLK (NDIRECT + NINDIRECT)

#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200

struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE } type;
    int ref;
    char readable;
    char writable;
    struct pipe *pipe;
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
    int64_t (*read)(struct inode*, uint8_t*, uint64_t);
    int64_t (*write)(struct inode*, uint8_t*, uint64_t);
};

extern struct dev_sw devs[];

struct file* file_dup(struct file *f);
int64_t file_read(struct file *f, char *addr, uint64_t n);
int64_t file_write(struct file *f, char *addr, uint64_t n);
void file_close(struct file *f);
struct file* file_alloc(void);
int file_stat(struct file *f, struct stat *st) ;

#define CONSOLE_DEVNUM 1