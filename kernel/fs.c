#include <stdint.h>
#include "bio.h"
#include "fs.h"
#include "params.h"
#include "file.h"
#include "string.h"
#include "stat.h"
#include "proc.h"
#include "debug.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

struct super_block sb;

void read_sb(uint64_t dev, struct super_block *sb) {
    struct buf *bp = bread(dev, 1);
    memcpy(sb, bp->data, sizeof(struct super_block));
    brelse(bp);
}

static void bzero(uint64_t dev, uint64_t bno) {
    struct buf *bp = bread(dev, bno);
    memset(bp->data, 0, BSIZE);
    bwrite(bp);
    brelse(bp);
}

static uint64_t balloc(uint64_t dev) {
    for(uint64_t b=0; b<sb.size; b += BPB) {
        struct buf *bp = bread(dev, BBLOCK(b, sb));
        // bp is a bitmap block that holds bth data block
        for(uint64_t bi=0; bi < BPB && b + bi < sb.size; bi++) {
            uint64_t m = 1 << (bi % 8);
            // which bit in the byte?
            if((bp->data[bi/8] & m) != 0) continue; 
            // this block is not free. continue

            bp->data[bi/8] |= m;
            bwrite(bp);
            brelse(bp);
            bzero(dev,b+bi);
            return b+bi;
        }
        brelse(bp);
    }
    panic("balloc: oob");
}

static void bfree(uint64_t dev, uint64_t blk) {
    struct buf *bp = bread(dev, BBLOCK(blk, sb));
    uint64_t bi = blk % BPB; // which bit in the block?
    uint64_t m = 1 << (bi % 8); // mask
    
    if((bp->data[bi/8] & m) == 0) {
        panic("freeing free block");
    }

    bp->data[bi/8] &= ~m;
    bwrite(bp);
    brelse(bp);
}

// Inodes

struct  {
    struct spin_lock lock;
    struct inode inodes[NINODE];
} icache;

void iinit(uint64_t dev) {
    init_lock(&icache.lock, "icache");
    for(int i=0; i<NINODE; i++) {
        init_sleep_lock(&icache.inodes[i].lock, "inode");
    }

    read_sb(dev, &sb);
}

// Alloc an inode on the disk
// Mark it as allocated by giving it type
// 
static struct inode* iget(uint64_t dev, uint64_t inum);
struct inode* ialloc(uint64_t dev, short type) {
    for(int inum = 1; inum <sb.n_inodes; inum++) {
        struct buf *bp = bread(dev, IBLOCK(inum, sb));
        struct dinode *dip = (struct dinode*)bp->data + inum % IPB;
        if(dip->type == 0) { // free
            memset(dip, 0, sizeof(struct dinode));
            dip->type = type;
            bwrite(bp);
            brelse(bp);
            return iget(dev, inum);
        }
        brelse(bp);
    }
    panic("ialloc: no inodes");
}

// write-through inode to disk
// must be called after every change to an ip->xxx field
// that lives on disk, since inode cache is write-through
// caller must hold ip->lock
void iupdate(struct inode *ip) {
    struct buf *bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    struct dinode *dip = (struct dinode*)bp->data + ip->inum % IPB;
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size = ip->size;
    memcpy(dip->addrs, ip->addrs, sizeof(ip->addrs));
    bwrite(bp);
    brelse(bp);
}

// Find the inode with number inum on cache
// and return the icache entry, not sleep-locked
static struct inode* iget(uint64_t dev, uint64_t inum) {
    acquire(&icache.lock);

    struct inode *empty = 0;
    
    struct inode *ip;
    for(int i=0; i<NINODE; i++) {
        ip = &icache.inodes[i];
        if(ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            // xv6 treats ref=0 inode as empty
            ip->ref++;
            release(&icache.lock);
            return ip;
        }

        // if ref=0, just evict
        if(empty == 0 && ip->ref == 0){
            empty = ip;
        }
    }

    if(empty == 0) {
        panic("iget: no empty slot");
    }

    ip = empty;
    ip->dev = dev;
    ip->inum = inum;
    ip->ref = 1;
    ip->valid = 0;
    release(&icache.lock);

    return ip;
}

struct inode* idup(struct inode *ip) {
    acquire(&icache.lock);
    ip->ref++;
    release(&icache.lock);
    return ip;
}

// Lock the inode and reads the inode from disk if necessary
void ilock(struct inode *ip) {
    if(ip == 0 || ip->ref < 1) {
        panic("ilock");
    }

    acquire_sleep(&ip->lock);

    if(ip->valid != 0) return;

    struct buf *bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    struct dinode *dip = (struct dinode*)bp->data + ip->inum % IPB;

    ip->type = dip->type;
    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;

    memcpy(ip->addrs, dip->addrs, sizeof(ip->addrs));
    brelse(bp);
    ip->valid = 1;
    if(ip->type == 0) {
        panic("ilock: no type");
    }
}

void iunlock(struct inode *ip) {
    if(ip == 0 || !holding_sleep(&ip->lock) || ip->ref < 1) {
        panic("iunlock");
    }

    release_sleep(&ip->lock);
}

// discard inode file's contents
// only called when nlink == 0, ref == 0
static void itrunc(struct inode *ip) {
    for(int i=0; i<NDIRECT; i++) {
        if(ip->addrs[i]) {
            bfree(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }

    if(ip->addrs[NDIRECT]) {
        struct buf *bp = bread(ip->dev, ip->addrs[NDIRECT]);
        uint64_t* addr = (uint64_t*)bp->data;
        for(int j=0; j < NINDIRECT; j++) {
            if(addr[j]) {
                bfree(ip->dev, addr[j]);
            }
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }
    ip->size = 0;
    iupdate(ip);
}

void iput(struct inode *ip) {
    acquire_sleep(&ip->lock);
    if(ip->valid && ip->nlink == 0) {
        acquire(&icache.lock);
        uint64_t r = ip->ref;
        release(&icache.lock);
        if(r==1) {
            itrunc(ip);
            ip->type = 0;
            iupdate(ip);
            ip->valid = 0;
        }
    }
    release_sleep(&ip->lock);

    acquire(&icache.lock);
    ip->ref--;
    release(&icache.lock);
}


// Return the disk block address of the nth block in the inode
// If none, alloc
static uint64_t bmap(struct inode* ip, uint64_t bn) {
    uint64_t addr;
    if(bn < NDIRECT) {
        addr = ip->addrs[bn];
        if(addr == 0) {
            ip->addrs[bn] = addr = balloc(ip->dev);
        }
        return addr;
    }

    bn -= NDIRECT;

    if(bn < NINDIRECT) {
        addr = ip->addrs[NDIRECT];
        if(addr == 0) {
            ip->addrs[NDIRECT] = addr = balloc(ip->dev);
        }
        struct buf *bp = bread(ip->dev, addr);
        uint64_t *a = (uint64_t*)bp->data;
        addr = a[bn];
        if(addr == 0) {
            a[bn] = addr = balloc(ip->dev);
            bwrite(bp);
        }
        brelse(bp);
        return addr;
    }
    panic("bmap: out of range");
}

// Caller must hold ip->lock
void stati(struct inode* ip, struct stat *st) {
    st->dev = ip->dev;
    st->ino = ip->inum;
    st->type = ip->type;
    st->nlink = ip->nlink;
    st->size = ip->size;
}

// it will not ilock, lock inode yourself
int64_t readi(struct inode *ip, char *dst, uint64_t off, uint64_t n) {
    if(ip->type == T_DEV) {
        if(ip->major < 0 || ip->major >= NDEV || !devs[ip->major].read) {
            return -1;
        }

        return devs[ip->major].read(ip, dst, n);
    }

    if(off > ip->size || off + n < off) {
        // check overflow
        return -1;
    }
    if(off + n > ip->size) {
        n = ip->size - off;
    }

    uint64_t m;
    uint64_t tot;
    for(tot=0; tot < n; tot += m, off += m, dst += m) {
        struct buf *bp = bread(ip->dev, bmap(ip, off/BSIZE));
        m = min(n - tot, BSIZE - off % BSIZE);
        memcpy(dst, bp->data + off % BSIZE, m);
        brelse(bp);
    }
    return tot; // why xv6 returns n?
}

// it will not ilock, lock inode yourself
int64_t writei(struct inode *ip, char *src, uint64_t off, uint64_t n) {
    if(ip->type == T_DEV) {
        if(ip->major < 0 || ip->major >= NDEV || !devs[ip->major].write) {
            return -1;
        }
        return devs[ip->major].write(ip, src, n);
    }

    if(off > ip->size || off + n < off) {
        // overflow
        return -1;
    }

    if(off + n > MAXFILEBLK * BSIZE) {
        // theoretically impossible
        return -1;
    }

    uint64_t m;
    uint64_t tot;
    for(tot=0; tot < n; tot += m, off += m, src += m) {
        struct buf *bp = bread(ip->dev, bmap(ip, off/BSIZE));
        m = min(n - tot, BSIZE - off % BSIZE);
        memcpy(bp->data + off % BSIZE, src, m);
        bwrite(bp);
        brelse(bp);
    }

    if(n > 0 && off > ip->size) {
        ip->size = off;
        iupdate(ip); // write to disk
    }

    return tot;
}

// Look for a dir entry in a dir
// If found, set *poff to byte offset of entry
struct inode* dir_lookup(struct inode *dp, char *name, uint64_t *poff) {
    if(dp->type != T_DIR) {
        // wtf
        panic("dir_lookup not on dir");
    }

    struct dir_ent de;
    for(uint64_t off = 0; off < dp->size; off += sizeof(struct dir_ent)) {
        if(readi(dp, (char*)&de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
            panic("dir_lookup can't read");
        }

        if(de.inum == 0) continue;
        // empty

        if(strncmp(name, de.name, DIRSIZ) == 0) {
            //match
            if(poff) *poff= off;
            uint64_t inum = de.inum;
            return iget(dp->dev, inum); //  return inode without lock

            // if we lookup ".", and if iget returns inode locked,
            // iget will never return because the inode lock is already hold within this call rn.
            // This is why iget returns the inode without sleeplocked
        }
    }

    return 0;
}

int dir_link(struct inode *dp, char *name, uint64_t inum) {
    // check no name collision
    struct inode *ip = dir_lookup(dp, name, 0);
    if(ip != 0) {
        // wtf collision
        iput(ip); // forfeit
        return -1;
    }

    struct dir_ent de;
    uint64_t off;
    for(off = 0; off < dp->size; off += sizeof(struct dir_ent)) {
        if(readi(dp, (char*)&de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
            panic("dir_link can't read");
        }
        if(de.inum == 0) break; //found
    }

    strncpy(de.name, name, DIRSIZ);
    de.inum = inum;
    if(writei(dp, (char*)&de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
        panic("dir_link cant write");
    }

    return 0;

}

// "///a//bb"
// 1. skip all /
// "///a//bb"
//     ^path
// 2. go til the name ends
// "///a//bb"
//      ^path
// 3. skip all /
// "///a//bb"
//        ^path
static char* skip_elem(char *path, char *name) {
    while(*path == '/') path++; //skip all '/'
    if(*path == 0) return 0;
    char *s = path; // finally, some letter not a /
    while(*path != '/' && *path != 0) path++; // go till the name ends
    int len = path - s; 
    if(len >= DIRSIZ) {
        memcpy(name, s, DIRSIZ);
    } else {
        memcpy(name, s, len);
        name[len] = 0;
    }

    while(*path == '/') path++; // skip all '/'
    return path;
}

static struct inode* namex(char *path, int namei_parent, char *name) {
    struct inode *ip = (*path == '/') ? iget(ROOTDEV, ROOTINO) : idup(myproc()->cwd);

    while((path = skip_elem(path, name)) != 0) {
        ilock(ip);
        if(ip->type != T_DIR) {
            // if it is not dir, we can't look it up
            iunlock(ip);
            iput(ip);
            return 0;
        }
        if(namei_parent && *path == '\0') {
            // namei_parent is true means, we need the path's target inode's parent
            // mkdir /a/b/c <- we need b's inode
            // ip -> b, path == 0, name == c, we throw away the name.
            iunlock(ip);
            return ip;
        }
        struct inode *next;
        if((next = dir_lookup(ip, name, 0)) == 0) { // set next to the name's ip
            // couldn't find it
            iunlock(ip);
            iput(ip);
            return 0;
        }
        iunlock(ip); // 
        iput(ip);
        ip = next;
    }
    return ip;
}

struct inode* namei(char *path) {
    char name[DIRSIZ];
    return namex(path, 0, name);
}

struct inode* namei_parent(char *path, char *name) {
    return namex(path, 1, name);
}