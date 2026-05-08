#include <stdint.h>
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "params.h"
#include "driver/ramdisk.h"
#include "debug.h"

struct {
    struct spin_lock lock;
    struct buf bufs[NBUF];
    struct buf head;
} bcache;

void bcache_init(void) {

    init_lock(&bcache.lock, "bcache");

    // create doublely linkned list
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    // make an empty list
    // head - head
    for(int i=0; i<NBUF; i++) {
        struct buf *b = &bcache.bufs[i];
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        // insert this buf at the next of the head
        // head - head.next
        //     \   /
        //       b
        init_sleep_lock(&b->lk, "buffer");

        bcache.head.next->prev = b;
        bcache.head.next = b;
        // head -x- head.next
        //      \   /
        //         b
    }
}

// Return the locked buffer, if there is none, just alloc it
static struct buf* bget(uint64_t dev, uint64_t block_no) {
    acquire(&bcache.lock);

    // is it here?
    for(struct buf *b = bcache.head.next; b != &bcache.head; b = b->next) {
        if(b->dev != dev || b->block_no != block_no) continue;
        b->ref_count++;
        release(&bcache.lock);
        acquire_sleep(&b->lk);
        return b;
    }

    // ok it is not at the cache.
    // look for the least recently used first
    for(struct buf *b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if(b->ref_count != 0) continue;
        // refcnt > 0 means at least one process is acessing the buf, and 
        // the possible others are sleeping for it
        // They expect this buffer to hold the block whose block_no is equal to the one they requested
        // but if we evict it, it would not hold the block that they requested.
        b->ref_count = 1; 

        release(&bcache.lock); 
        // We are gonna sleep, so we need to release any spinlock
        // we done editing bcache fields anyway
        acquire_sleep(&b->lk);

        // We only check the refcnt, cuz we dont have logging
        // so we can evict the dirty block

        if(b->flags & B_DIRTY) {
            rd_rw(b);
        }

        b->dev = dev;
        b->block_no = block_no;
        b->flags = 0; // no valid

        return b;
    }

    panic("bget: no buffers available");

}

// return a locked buf with the contents of the block
struct buf* bread(uint64_t dev, uint64_t blk_no) {
    struct buf *b = bget(dev, blk_no);
    if((b->flags & B_VALID) == 0) {
        rd_rw(b);
    }
    return b;
}

void bwrite(struct buf *b) {
    if(!holding_sleep(&b->lk)) {
        // if this process is not holding the sleeplock,
        // it didn't get the buf from bget
        panic("bwrite : no sleeplk holded");
    }
    b->flags |= B_DIRTY;
    // rd_rw(b); writeback
}

void brelse(struct buf *b) {
    if(!holding_sleep(&b->lk)) {
        panic("brelease: wtf");
    }

    release_sleep(&b->lk);
    
    acquire(&bcache.lock);
    b->ref_count--;
    if(b->ref_count == 0) {
        // no one is waiting for it
        b->next->prev = b->prev;
        b->prev->next = b->next;
        // b.prev - b.next
        // b popped
        
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        // head - head.next
        //   \      /
        //       b

        bcache.head.next->prev = b;
        bcache.head.next = b;
        // head - b - head.next
        // most recently used

    }
    release(&bcache.lock);
}

void test_bcache(void) {
    // test bread/brelese
    struct buf *b = bread(DEV_RAMDISK, 0);
    if((b->flags & B_VALID) == 0) panic("no valid bit on buf");
    // brelse(b);

    // struct buf *b1 = bread(DEV_RAMDISK, 0);
    b->data[0] = 0xAB;
    b->data[1] = 0xCD;
    bwrite(b);
    brelse(b);

    struct buf *b2 = bread(DEV_RAMDISK, 0);
    if((b2->data[0] != 0xAB) || (b2->data[1] != 0xCD)) {
        panic("wtf");
    }

    brelse(b2);

    // I will evict that buf and check the disk
    struct buf *bufs[NBUF];
    for(int i=0; i<NBUF; i++) {
        bufs[i] = bread(0, i+10);
    }
    for(int i=0; i<NBUF; i++) {
        brelse(bufs[i]);
    }

    uint8_t* rd = get_ramdisk();
    if(rd[0] != 0xAB || rd[1] != 0xCD) {
        panic("why not write on disk");
    }


}