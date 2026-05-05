#include <stdint.h>

void bcache_init(void);
static struct buf* bget(uint64_t dev, uint64_t block_no);
struct buf* bread(uint64_t dev, uint64_t blk_no);
void bwrite(struct buf *b);
void brelse(struct buf *b);
void test_bcache(void);