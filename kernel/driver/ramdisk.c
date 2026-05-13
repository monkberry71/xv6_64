#include <stdint.h>
#include "../fs.h"
#include "../string.h"
#include "../debug.h"

static uint8_t *ramdisk;
static uint64_t disk_size;

void rd_init(void) {
    extern uint8_t _binary_build_fs_img_start[];
    extern uint8_t _binary_build_fs_img_size[];

    ramdisk = _binary_build_fs_img_start;
    disk_size = (uint64_t)_binary_build_fs_img_size;
}

void rd_intr(void) {

}

void rd_rw(struct buf *b) {
    // !!!!!!! if not locked, pannic
    if((b->flags & (B_VALID | B_DIRTY)) == B_VALID) {
        // valid but clean, why did you call me in the first place?
        panic("rd_rw: nothing to do");
    }

    if(b->dev != DEV_RAMDISK) {
        panic("rd_rw: request not for rd");
    }

    if(b->block_no >= disk_size/BSIZE) {
        panic("rd_rw: block out of range");
    }

    char *p = ramdisk + b->block_no * BSIZE;

    if(b->flags & B_DIRTY) {
        // we got it written, so it is clean 
        b->flags &= ~B_DIRTY;
        memcpy(p, b->data, BSIZE);
    } else {
        // non_valid and clean -> read from the disk
        memcpy(b->data, p, BSIZE);
    }

    b->flags |= B_VALID; // anyway, we synced it.
}

uint8_t* get_ramdisk(void) {
    return ramdisk;
}