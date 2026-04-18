#include <stdint.h>


struct mb2_tag {
    uint32_t type;
    uint32_t size;
};

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
    struct mb2_tag tags[];
};

struct mb2_tag_fb {
    struct mb2_tag tag;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  fb_type;
    uint16_t reserved;
};

struct mb2_tag_mm {
    struct mb2_tag tag;
    uint32_t entry_size;
    uint32_t entry_version;
};

#define MB2_FOREACH_TAG(mb2_phys, tag) \
    for ( \
        (tag) = (struct mb2_tag *)((uint64_t)(mb2_phys) + 8); \
        (tag)->type != 0; \
        (tag) = (struct mb2_tag *)(( (uint64_t)(tag) + (tag)->size + 7 ) & ~( (uint64_t)7 ))\
    )

void preserve_mb2(uint32_t mb2_info_phys);