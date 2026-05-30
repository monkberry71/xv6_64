#include <stdint.h>
#include "proc.h"
#include "file.h"
#include "params.h"
#include "fs.h"
#include "debug.h"
#include "string.h"
#include "pipe.h"

#define CHECKFD(fd) if((fd) < 0 || (fd) >= NOFILE || (myproc()->ofile[(fd)]) == 0) return -1

static int fd_alloc(struct file *f) {
    struct proc *cur_p = myproc();

    for(int fd = 0; fd < NOFILE; fd++) {
        if(cur_p->ofile[fd] == 0) {
            cur_p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

int64_t sys_dup(void) {
    int old_fd = myproc()->rp->rdi;
    CHECKFD(old_fd);

    struct file *f = myproc()->ofile[old_fd];
    // if(old_fd < 0 || old_fd >= NOFILE || f == 0) {
    //     return -1;
    // }
    int fd = fd_alloc(f);
    if(fd < 0) return -1;
    file_dup(f);
    return fd;
}

int64_t sys_read(void) {
    int fd = myproc()->rp->rdi;
    CHECKFD(fd);

    char *p = (void*)myproc()->rp->rsi;
    uint64_t n = myproc()->rp->rdx;

    struct file *f = myproc()->ofile[fd];

    return file_read(f, p, n);
}

int64_t sys_write(void) {
    int fd = myproc()->rp->rdi;
    CHECKFD(fd);
    char *p = (void*)myproc()->rp->rsi;
    uint64_t n = myproc()->rp->rdx;

    struct file *f = myproc()->ofile[fd];

    return file_write(f, p, n);
}

int64_t sys_close(void) {
    int fd = myproc()->rp->rdi;
    CHECKFD(fd);
    struct file *f = myproc()->ofile[fd];
    myproc()->ofile[fd] = 0;
    file_close(f);
    return 0;
}

int64_t sys_fstat(void) {
    int fd = myproc()->rp->rdi;
    CHECKFD(fd);
    struct stat *st = (void*)myproc()->rp->rsi;
    struct file *f = myproc()->ofile[fd];

    if(st == 0) return -1;
    return file_stat(f, st);
}

// create locked inode of the path
static struct inode* create(char *path, short type, short major, short minor) {
    char name[DIRSIZ];

    struct inode *dp = namei_parent(path, name);
    if(dp == 0) return 0;

    ilock(dp);
    
    struct inode *ip = dir_lookup(dp, name, 0);

    if(ip != 0) {
        iunlock(dp);
        iput(dp);
        ilock(ip);
        if(type == T_FILE && ip->type == T_FILE) return ip;
        iunlock(ip); 
        iput(ip);
        return 0;
    }

    // if there is none, make one
    ip = ialloc(dp->dev, type);
    if(ip == 0) {
        panic("create: ialloc failed");
    }

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);

    if(type == T_DIR) {
        dp->nlink++; // for ..
        iupdate(dp);

        // make our good old . and .. 
        if(dir_link(ip, ".", ip->inum) < 0 || dir_link(ip, "..", dp->inum) < 0) {
            panic("create: dots");
        }
    }

    if(dir_link(dp, name, ip->inum) < 0) {
        panic("create: cant belong to the dir");
    }
    iunlock(dp);
    iput(dp);

    return ip;
}

int64_t sys_open(void) {
    char *path = (void*) myproc()->rp->rdi;
    uint64_t omode = myproc()->rp->rsi;

    struct inode* ip;

    if(omode & O_CREATE) {
        //create
        ip = create(path, T_FILE, 0, 0);
        if(ip == 0) {
            return -1;
        }
    } else {
        // edit
        ip = namei(path);
        if(ip == 0) {
            return -1;
        }
        ilock(ip);
        if(ip->type == T_DIR && omode != O_RDONLY) {
            // open cannot edit dir
            iunlock(ip);
            iput(ip);
            return -1;
        }
    }

    struct file* f = file_alloc();
    if(f == 0) {
        iunlock(ip);
        iput(ip);
        return -1;
    }

    int fd = fd_alloc(f);
    if(fd < 0) {
        file_close(f);
        iunlock(ip);
        iput(ip);
        return -1;
    }

    iunlock(ip);
    
    // we dont need the ftable lock, because when file_alloc, with lock, it increase the ref, so other thread wont pick it
    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY); // wr only on -> no read
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR); // WRONLY or RDRW -> writable

    return fd;
}

int64_t sys_mkdir(void) {
    char *path = (void*) myproc()->rp->rdi;
    
    struct inode *ip = create(path, T_DIR, 0, 0);
    if(ip == 0) return -1;
    iunlock(ip);
    iput(ip);
    return 0;
}

int64_t mknod_con(void) {
    // just for testing
    struct inode *ip = create("/console", T_DEV, 1, 1);
    if(ip == 0) return 0;
    iunlock(ip);
    iput(ip);
    return 0;
}

int64_t sys_mknod(void) {
    char *path = (void*) myproc()->rp->rdi;
    uint64_t major, minor;
    major = myproc()->rp->rsi;
    minor = myproc()->rp->rdx;

    struct inode *ip = create(path, T_DEV, major, minor);
    if(ip == 0) return -1;
    iunlock(ip);
    iput(ip);
    return 0;
}

int64_t sys_chdir(void) {
    char *path = (void*) myproc()->rp->rdi;
    struct inode *ip = namei(path);
    if(ip == 0) return -1;

    ilock(ip);
    if(ip->type != T_DIR) {
        // chdir to non-dir... 
        iunlock(ip);
        iput(ip);
        return -1;
    }
    iunlock(ip);

    iput(myproc()->cwd);
    myproc()->cwd = ip;
    return 0;
}

int64_t exec(char *path, char **argv);
int64_t sys_exec(void) {
    char *path = (void*) myproc()->rp->rdi;
    char **argv = (void*) myproc()->rp->rsi;

    return exec(path, argv);
}

static int is_dir_empty(struct inode *dp) {
    for(int off = 2 * sizeof(struct dir_ent); off < dp->size; off += sizeof(struct dir_ent)) {
        // skip ".", ".."
        struct dir_ent de;
        if(readi(dp, (char*) &de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
            panic("is_dir_empty: cannot read");
        }
        if(de.inum != 0) return 0;
    }
    return 1;
}

int64_t sys_unlink(void) {
    char *path = (void*) myproc()->rp->rdi;
    
    char name[DIRSIZ];
    struct inode *dp = namei_parent(path, name);
    if(dp == 0) return -1;

    ilock(dp);

    if(strncmp(name, ".", DIRSIZ) == 0 || strncmp(name, "..", DIRSIZ) == 0) 
        goto bad;
    
    uint64_t off;
    struct inode *ip = dir_lookup(dp, name, &off);
    if(ip == 0) 
        goto bad;
    ilock(ip);

    if(ip->nlink < 1) {
        panic("unlink: nlink < 1");
    }

    if(ip->type == T_DIR && !is_dir_empty(ip)) {
        iunlock(ip);
        iput(ip);
        goto bad;
    }

    struct dir_ent de;
    memset(&de, 0, sizeof(de));
    if(writei(dp, (char*)&de, off, sizeof(struct dir_ent)) != sizeof(struct dir_ent)) {
        panic("unlink: writei");
    }

    if(ip->type == T_DIR) {
        // ip's .. is dp
        dp->nlink--;
        iupdate(dp);
    }

    iunlock(dp);
    iput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlock(ip);
    iput(ip);

    return 0;

bad:
    iunlock(dp);
    iput(dp);
    return -1;
}

int64_t sys_pipe(void) {
    int *fd = (void*) myproc()->rp->rdi;

    struct file *rf, *wf;
    if(pipe_alloc(&rf, &wf) < 0) {
        return -1;
    }

    int fd0 = fd_alloc(rf);
    if(fd0 < 0) {
        file_close(rf);
        file_close(wf);
        return -1;
    }

    int fd1 = fd_alloc(wf);
    if(fd1 < 0) {
        myproc()->ofile[fd0] = 0;
        file_close(rf);
        file_close(wf);
        return -1;
    }
    
    fd[0] = fd0;
    fd[1] = fd1;

    return 0;
}