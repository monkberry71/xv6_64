#pragma once

#define PGSIZE_4KB 4096
#define PGSIZE_2MB (2 * 1024 * 1024) 
#define PGSIZE_1GB (1ULL * 1024 * 1024 * 1024)

#define ROUNDUP(sz, size) (((sz) + (size) - 1) & ~((uint64_t)(size) -1))