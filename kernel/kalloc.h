#include <stdint.h>

void* kalloc(void);
void kinit(void);
void free_range(void *vstart, void* vend);
void kfree(char* v);