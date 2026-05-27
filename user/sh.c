#include <stdint.h>
#include "user.h"

#define MAXARGS 16

static int
parsecmd(char *s, char **argv)
{
    int argc = 0;

    while(*s) {
        while(*s == ' ' || *s == '\t' || *s == '\n')
            *s++ = 0;

        if(*s == 0)
            break;

        if(argc >= MAXARGS - 1)
            break;

        argv[argc++] = s;

        while(*s && *s != ' ' && *s != '\t' && *s != '\n')
            s++;
    }

    argv[argc] = 0;
    return argc;
}

int
main(void)
{
    char buf[128];
    char *argv[MAXARGS];

    for(;;) {
        write(1, "$ ", 2);

        int n = read(0, buf, sizeof(buf) - 1);
        if(n <= 0)
            exit();

        buf[n] = 0;

        int argc = parsecmd(buf, argv);
        if(argc == 0)
            continue;

        int pid = fork();
        if(pid < 0) {
            printf(1, "fork failed\n");
            continue;
        }

        if(pid == 0) {
            exec(argv[0], argv);
            printf(1, "exec failed: %s\n", argv[0]);
            exit();
        }

        wait();
    }
}