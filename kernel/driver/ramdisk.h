#pragma once
#include "../fs.h"

void rd_init(void);
void rd_rw(struct buf *b);
char* get_ramdisk(void); // for test