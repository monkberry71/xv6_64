#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// this is some sort of script prog that makes fs.img
// 

#define NDIRECT 13
#define NINDIRECT (BSIZE / sizeof(uint64_t))
#define ROOTINO 1 // root dir inode
#define BSIZE 4096
#define DISK_SIZE (16 * 1024 * 1024) // 16*2^20 == 16MB
#define NBLOCKS (DISK_SIZE / BSIZE)

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

#define BPB (BSIZE*8)
#define BBLOCK(b, sb) ( (b)/BPB + (sb)->bmap_start )

#define DIRSIZ 24

#define min(a,b) ((a) < (b) ? (a) : (b))

#define CONSOLE_DEVNUM 1


struct super_block {
    uint64_t size; // Size of fs image (in blocks)
    uint64_t n_blocks; // num of data blocks
    uint64_t n_inodes; // num of inodes(not block, inode count)
    // uint64_t n_logs
    uint64_t inode_start; // block_no of first inode block
    uint64_t bmap_start; // block_no of first free map block
};

struct dinode {
    short type; // 
    short major; // 
    short minor; 
    short nlink;
    uint64_t size;
    uint64_t addrs[NDIRECT+1];
};

struct dir_ent {
    uint64_t inum; // 8byte
    char name[DIRSIZ];
};

typedef struct block {
    char bytes[BSIZE];
} Block;

// uint8_t disk[DISK_SIZE];
Block disk[NBLOCKS];
struct super_block *sb = (void*) &disk[1];

// get data block
int balloc(void) {
    static int bump = 6; // bump block addr
    Block *bitmap_block = (void*)&disk[BBLOCK(bump,sb)];

    uint64_t inblock_byte_index = bump / 8;
    uint64_t inbyte_bit_index = bump % 8;

    bitmap_block->bytes[inblock_byte_index] |= 1 << (inbyte_bit_index);

    return bump++;
}

struct dinode* inodes = (void*) &disk[3];
int dialloc(short type) {
    static int bump = 1; // root inode
    
    struct dinode* to_alloc = &inodes[bump];
    to_alloc->type = type;
    return bump++;
}

uint64_t bmap(struct dinode *ip, uint64_t bn) {
    uint64_t addr;
    if(bn < NDIRECT) {
        addr = ip->addrs[bn];
        if(addr == 0) {
            ip->addrs[bn] = addr = balloc();
        }
        return addr;
    }

    bn -= NDIRECT; 

    if(bn < NINDIRECT) {
        addr = ip->addrs[NDIRECT];
        if(addr == 0) {
            ip->addrs[NDIRECT] = addr = balloc();
        }
        uint64_t *a = (void*) &disk[addr];
        addr = a[bn];
        if(addr == 0) {
            a[bn] = addr = balloc();
        }
        return addr;
    }

    fprintf(stderr, "bmap oor");
    exit(1);
}

int64_t readi(struct dinode *ip, char *dst, uint64_t off, uint64_t n) {
    if(off > ip->size || off + n < off) {
        return -1;
    }

    if(off + n > ip->size) {
        n = ip->size - off;
    }

    uint64_t m, tot;
    for(tot=0; tot < n; tot += m, off += m, dst += m) {
        Block *b = &disk[bmap(ip, off/BSIZE)];
        m = min(n - tot, BSIZE - off % BSIZE);
        memcpy(dst, b->bytes + off % BSIZE, m);
    }

    return tot;
}

int64_t writei(struct dinode *ip, char *src, uint64_t off, uint64_t n) {
    if(off > ip->size || off + n < off) {
        return -1;
    }

    if(off +n > (NDIRECT + NINDIRECT) * BSIZE) {
        return -1;
    }

    uint64_t m, tot;
    for(tot =0; tot < n; tot += m, off += m, src += m) {
        Block *b = &disk[bmap(ip, off/BSIZE)];
        m = min(n - tot, BSIZE - off % BSIZE);
        memcpy(b->bytes + off % BSIZE, src, m);
    }

    if(n > 0 && off > ip->size) {
        ip->size = off;
    }

    return tot;
}

int dir_link(struct dinode *dp, char *name, uint64_t inum) {
    struct dir_ent de;
    uint64_t off;
    for(off=0; off < dp->size; off += sizeof(struct dir_ent)) {
        if(readi(dp, (void*) &de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
            fprintf(stderr, "dir_link cant read");
            exit(1);
        }
        if(de.inum == 0) break; // found
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;

    if(writei(dp, (void*)&de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
        fprintf(stderr, "dir_link cant write");
        exit(1);
    }

    return 0;
}

int file2i(struct dinode *ip, char* file_name_on_host) {
    if(ip->type != T_FILE) return -1;

    FILE *f = fopen(file_name_on_host, "rb");
    if(f == 0) return -1;

    char buf[BSIZE];
    uint64_t off = 0;

    for(;;) {
        uint64_t n = fread(buf, 1, sizeof(buf), f);
        if(n > 0) {
            if(writei(ip, buf, off, n) != (int64_t) n) {
                fclose(f);
                return -1;
            }
            off += n;
        }

        if(n < sizeof(buf)) {
            if(ferror(f)) {
                fclose(f);
                return -1;
            }
            break;
        }

    }
    fclose(f);
    return 0;
    
}

int save_img() {
    FILE *f = fopen("fs.img", "wb");
    if(f == 0){
        return -1;
    }

    fwrite(disk, sizeof(disk), 1, f);
    fclose(f);
    return 0;
}

int main(void) {

    sb->size = NBLOCKS;
    sb->n_blocks = (NBLOCKS -(1 + 1 + 1 + 3));
    sb->n_inodes = (3*BSIZE) / sizeof(struct dinode);
    sb->inode_start = 3;
    sb->bmap_start = 2;

    // | zero | sb | bmap | inode0 | inode1 | inode2 | data blocks ... 

    disk[sb->bmap_start].bytes[0] = (1 << 6) - 1; // check bitmap

    // make root dir inode
    struct dinode *rooti = &inodes[dialloc(T_DIR)];
    rooti->nlink = 1;
    dir_link(rooti, ".", ROOTINO);
    dir_link(rooti, "..", ROOTINO);

    int console_ino = dialloc(T_DEV);
    struct dinode *console = &inodes[console_ino];
    console->nlink = 1;
    console->major = 1;
    console->minor = 1;
    dir_link(rooti, "console", console_ino);

    // add init elf
    int init_ino = dialloc(T_FILE);
    struct dinode *init = &inodes[init_ino];
    init->nlink = 1;
    if( file2i(init, "user/init") < 0 ) {
        fprintf(stderr, "file2i init failed");
        exit(1);
    }
    dir_link(rooti, "init", init_ino);

    // add sh elf
    int sh_ino = dialloc(T_FILE);
    struct dinode *sh = &inodes[sh_ino];
    sh->nlink = 1;
    if( file2i(sh, "user/sh") < 0 ) {
        fprintf(stderr, "file2i sh failed");
        exit(1);
    }
    dir_link(rooti, "sh", sh_ino);

    // add cat
    int cat_ino = dialloc(T_FILE);
    struct dinode *cat = &inodes[cat_ino];
    cat->nlink = 1;
    if( file2i(cat, "user/cat") < 0 ) {
        fprintf(stderr, "file2i cat failed");
        exit(1);
    }
    dir_link(rooti, "cat", cat_ino);



    save_img();

}


