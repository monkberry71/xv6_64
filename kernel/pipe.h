#include <stdint.h>

int pipe_alloc(struct file **f0, struct file **f1);
void pipe_close(struct pipe *p, int writable);
int64_t pipe_write(struct pipe *p, char *addr, uint64_t n);
int64_t pipe_read(struct pipe *p, char *addr, uint64_t n);