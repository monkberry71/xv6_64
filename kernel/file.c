#include <stdint.h>
#include "params.h"
#include "spinlock.h"
#include "file.h"
#include "fs.h"
#include "debug.h"
#include "stat.h"
#include "pipe.h"

struct dev_sw devs[NDEV];
struct {
    struct spin_lock lock;
    struct file files[NFILE];
} ftable;

void file_init(void) {
    init_lock(&ftable.lock, "ftable");
}

// Allocate a file structure
struct file* file_alloc(void) {
    // struct file *f;

    acquire(&ftable.lock);
    for(int i=0; i<NFILE; i++) {
        struct file *f = &ftable.files[i];
        if(f->ref == 0) {
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }
    release(&ftable.lock);
    return 0;
}

struct file* file_dup(struct file *f) {
    acquire(&ftable.lock);
    if(f->ref < 1) {
        panic("file_dup : dup what?");
    }

    f->ref++;
    release(&ftable.lock);
    return f;
}

// close file f == dec ref, close when 0
void file_close(struct file *f) {
    acquire(&ftable.lock);
    if(f->ref < 1) {
        panic("file_close: close what?");
    }
    if(--f->ref > 0) {
        release(&ftable.lock);
        return;
    }

    // f->ref was 1, now 0.
    struct file ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&ftable.lock);

    if(ff.type == FD_PIPE) {
        // pipe_close
        pipe_close(ff.pipe, ff.writable);
    } else if(ff.type == FD_INODE) {
        iput(ff.ip);
    }
}

// get metadata 
int file_stat(struct file *f, struct stat *st) {
    if(f->type == FD_INODE) {
        ilock(f->ip);
        stati(f->ip, st);
        iunlock(f->ip);
        return 0;
    }
    return -1;
}

int64_t file_read(struct file *f, char *addr, uint64_t n) {
    if(f->readable == 0) return -1;
    if(f->type == FD_PIPE) {
        // return pipe_read
        return pipe_read(f->pipe, addr, n);
    }
    if(f->type == FD_INODE) {
        ilock(f->ip);
        int64_t r = readi(f->ip, addr, f->off, n);
        if(r>0) {
            f->off += r;
        }
        iunlock(f->ip);
        return r;
    }
    panic("file_read failed");
}

int64_t file_write(struct file *f, char *addr, uint64_t n) {
    if(f->writable == 0) return -1;
    if(f->type == FD_PIPE) {
        return pipe_write(f->pipe, addr, n);
    }
    if(f->type == FD_INODE) {
        ilock(f->ip);
        int64_t r = writei(f->ip, addr, f->off, n);
        if(r>0) {
            f->off += r;
        }
        iunlock(f->ip);
        return r;
    }

    panic("file_write failed");
}