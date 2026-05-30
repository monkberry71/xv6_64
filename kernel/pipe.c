#include <stdint.h>
#include "spinlock.h"
#include "kalloc.h"
#include "file.h"
#include "proc.h"

#define PIPESIZE 512

struct pipe {
    struct spin_lock lock;
    char data[PIPESIZE];
    uint64_t nread;
    uint64_t nwrite;
    int read_open; // read fd is still open
    int write_open; // write fd is still open
};

int pipe_alloc(struct file **f0, struct file **f1) {
    struct pipe *p = 0;

    *f0 = *f1 = 0;

    *f0 = file_alloc();
    if(*f0 == 0) goto bad;

    *f1 = file_alloc();
    if(*f1 == 0) goto bad;

    p = kalloc();
    if(p == 0) goto bad;

    p->read_open = 1;
    p->write_open = 1;
    p->nwrite = 0;
    p->nread = 0;

    init_lock(&p->lock, "pipe");

    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = p;

    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = p;

    return 0; 

    bad:
    if(p) {
        kfree((void*) p);
    }
    if(*f0) {
        file_close(*f0);
    }
    if(*f1) {
        file_close(*f1);
    }
    return -1;
}

// it clears only pipe resources, called by file_close 
void pipe_close(struct pipe *p, int writable) {
    acquire(&p->lock);
    if(writable) {
        p->write_open = 0;
        wakeup(&p->nread);
    } else {
        p->read_open = 0;
        wakeup(&p->nwrite);
    }

    if(p->read_open == 0 && p->write_open == 0) {
        release(&p->lock);
        kfree((void*) p);
    } else {
        release(&p->lock);
    }
}

int64_t pipe_write(struct pipe *p, char *addr, uint64_t n) {
    acquire(&p->lock);
    for(int i=0; i < n; i++) {
        while(p->nwrite == p->nread + PIPESIZE) {
            // CV pattern
            if (p->read_open == 0 || myproc()->killed) {
                // no one to read, then why write?
                release(&p->lock);
                return -1;
            }

            wakeup(&p->nread); // I cant write cuz its full, anyone try to read? wakeup then
            sleep(&p->nwrite, &p->lock); // sleep until read happens
        }
        p->data[p->nwrite++ % PIPESIZE] = addr[i];
    }

    wakeup(&p->nread); // ok I wrote smth, anyone try to read?
    release(&p->lock);
    return n;
}

int64_t pipe_read(struct pipe *p, char *addr, uint64_t n) {
    acquire(&p->lock);
    
    // pipe read doesnt need to wait everytime when it tries to read.
    // if it is not empty, it can greedily read some
    // just read 1 byte and return
    while(p->nread == p->nwrite && p->write_open) {
        // CV pattern
        if(myproc()->killed) {
            release(&p->lock);
            return -1;
        }
        sleep(&p->nread, &p->lock);
    }

    int i;
    for(i=0; i<n; i++) {
        if(p->nread == p->nwrite) {
            //empty
            break;
        }
        addr[i] = p->data[p->nread++ % PIPESIZE];
    }

    wakeup(&p->nwrite);
    release(&p->lock);
    return i;
}