#include "fcntl.h"
#include "user.h"

char *argv[] = {"sh", 0};

int main(void) {
    if(open("/console", O_RDWR) < 0) {
        mknod("/console", 1, 1); // CONSOLE_DEVNUM 1
        open("/console", O_RDWR);
    }
    dup(0);
    dup(0);

    write(1, "init", 5);
}