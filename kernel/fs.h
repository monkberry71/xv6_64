#pragma once
#include <stdint.h>
#include "sleeplock.h"
#include "file.h"
#include "stat.h"

#define ROOTINO 1 // root dir inode
#define BSIZE 4096
#define DISK_SIZE (16 * 1024 * 1024) // 16*2^20 == 16MB
#define NBLOCKS (DISK_SIZE / BSIZE)

#define B_VALID 0x2
#define B_DIRTY 0x4

#define DEV_RAMDISK 0
#define ROOTDEV DEV_RAMDISK

struct super_block {
    uint64_t size; // Size of fs image (in blocks)
    uint64_t n_blocks; // num of data blocks
    uint64_t n_inodes; // num of inodes(not block, inode count)
    // uint64_t n_logs
    uint64_t inode_start; // block_no of first inode block
    uint64_t bmap_start; // block_no of first free map block
};

struct buf {
    uint64_t flags;
    uint64_t dev;
    uint64_t block_no;
    struct sleep_lock lk;
    uint32_t ref_count;
    struct buf *prev, *next, *qnext;
    uint8_t data[BSIZE];
};

// Bitmap bits per block,one blk can hold 4096 * 8 bits -> one blk can manage 4096 * 8 blks
#define BPB (BSIZE*8)

// Which bitmap block index holds the bit for data block b
#define BBLOCK(b, sb) ( (b)/BPB + (sb).bmap_start )


struct dinode {
    short type; // 
    short major; // 
    short minor; 
    short nlink;
    uint64_t size;
    uint64_t addrs[NDIRECT+1];
};
// NDIRECT 13
// 2 2 2 2 8 (8*14) 
// 8 * 16 = 2^7 = 128 bytes == sizeof(dinode)
// IPB == 4096 / 128 == 32
// 13 direct blocks + 1 indirect block
// 13 * 4096
// 1 indirect block == 4096 / 8 = 512 entries
// 512 * 4096 == 2^(9+12) == 2^21 == 2MB max file size

#define IPB (BSIZE / sizeof(struct dinode))
#define IBLOCK(i, sb) ((i) / IPB +(sb).inode_start)

#define DIRSIZ 24

struct dir_ent {
    uint64_t inum; // 8byte
    char name[DIRSIZ];
};

void iput(struct inode *ip);
void ilock(struct inode *ip);
void iunlock(struct inode *ip);
int64_t readi(struct inode *ip, char *dst, uint64_t off, uint64_t n);
int64_t writei(struct inode *ip, char *src, uint64_t off, uint64_t n);
void stati(struct inode* ip, struct stat *st);
struct inode* namei_parent(char *path, char *name) ;
struct inode* namei(char *path);
struct inode* dir_lookup(struct inode *dp, char *name, uint64_t *poff);
int dir_link(struct inode *dp, char *name, uint64_t inum);
struct inode* ialloc(uint64_t dev, short type);
void iupdate(struct inode *ip);

void fs_init(uint64_t dev) ;