#include <stdint.h>
#include "mb2.h"
#include "vm.h"
#include "debug.h"
#include "spinlock.h"

extern struct mb2_info* reserved_mb2_info;
struct {
    void* fb_base; // io remapped.
    uint64_t width, height, pitch;
    uint8_t bpp, type;
} gop_fb;


void gop_init(void) {
    struct mb2_tag* tag;
    int found = 0;
    MB2_FOREACH_TAG(reserved_mb2_info, tag) {
        if(tag->type != MB2_TAG_FB) continue;
        found = 1;

        struct mb2_tag_fb *fb_info = (void*)tag;
        
        if(fb_info->fb_type != 1) {
            panic("unsupported fb type");
        }

        gop_fb.pitch  = fb_info->pitch;
        gop_fb.width  = fb_info->width;
        gop_fb.height = fb_info->height;
        gop_fb.bpp    = fb_info->bpp;
        gop_fb.type   = fb_info->fb_type;

        uint64_t buffer_size = fb_info->pitch * fb_info->height;
        gop_fb.fb_base = io_remap((void*) fb_info->addr, buffer_size);

    }
    if(found == 0) {
        panic("No GOP found");
    }
}

void gop_draw_pixel(uint64_t x, uint64_t y, uint64_t color) {
    uint8_t bytes_per_pixel = gop_fb.bpp / 8;
    uint8_t *pixel = (uint8_t*) gop_fb.fb_base + y * gop_fb.pitch + x * bytes_per_pixel;
    for(uint8_t i=0; i<bytes_per_pixel; i++) {
        pixel[i] = (color >> (i * 8)) & 0xFF;
    }
}

void gop_draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t color) {
    // push_cli();
    uint64_t x_end = x + w > gop_fb.width ? gop_fb.width : x + w;
    uint64_t y_end = y + h > gop_fb.height ? gop_fb.height : y + h;
    for(uint64_t row = y; row < y_end; row++)
        for(uint64_t col = x; col < x_end; col++)
            gop_draw_pixel(col, row, color);
    // pop_cli();
}

// extern struct mb2_info* reserved_mb2_info;
// void kinit(void) {
//     kmem.free_list = 0;
//     struct mb2_tag* tag;
//     MB2_FOREACH_TAG(reserved_mb2_info, tag) {
//         if(tag->type != MB2_TAG_MMAP) continue;
        
//         struct mb2_tag_mm *mm_tag = tag;
//         uint64_t entry_length = (mm_tag->tag.size - sizeof(struct mb2_tag_mm)) / mm_tag->entry_size;
//         for(uint64_t i=0; i<entry_length; i++) {
//             struct mb2_mmap_entry *e = (struct mb2_mmap_entry*)((char*)mm_tag->entries + i * mm_tag->entry_size);
//             if(e->type != MB2_MMAP_AVAIL) continue;

//             // below bump is not free-able.
//             uint64_t mmap_start = e->base_addr;
//             uint64_t mmap_end = e->base_addr + e->length;
//             if(mmap_start < p_bump_end()) {
//                 mmap_start = p_bump_end();
//             }
            
//             free_range(P2V(mmap_start), P2V(mmap_end));
//         }
//     }
//     bump_off();
// }