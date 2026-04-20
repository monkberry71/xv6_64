#include <stdint.h>

void bump_init(void);
void* bump_alloc_page_4kb(void);
uint64_t p_bump_end(void);
void bump_off(void);