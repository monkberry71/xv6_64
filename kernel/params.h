#pragma once

#define NCPU 16
#define KSTACKSIZE 4096
#define NPROC 64
#define MAXOPBLOCKS 10
#define NBUF (MAXOPBLOCKS * 3)
#define NINODE 30
#define NDEV 10
#define NFILE 100
#define NOFILE 16
#define MAXARG 32 
// N open file

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))