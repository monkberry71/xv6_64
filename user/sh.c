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


    // printf(2, "pipe test : ");
    // int p[2];
    // pipe(p);
    // write(p[1], "x", 1);
    // read(p[0], buf, 1);
    // close(p[0]);
    // close(p[1]);
    // write(2, buf, 1);
    // printf(2, "\n");

    // printf(2, "pipe fork test : ");
    // int p[2];
    // pipe(p);
    // if(fork() == 0) {
    //     close(p[0]);
    //     write(p[1], "hi\n", 3);
    //     exit();
    // }
    // close(p[1]);
    // read(p[0], buf, 3);
    // write(2, buf, 3);
    // printf(2, "\n");
    // wait();

    printf(2, "pipe return test\n");
    int p[2];
    pipe(p);

    if(fork() == 0) {
        close(p[0]);
        close(p[1]);
        exit();
    }

    close(p[1]);
    int n = read(p[0], buf, 10);
    printf(2, "eof read=%d\n", n);
    close(p[0]);
    wait();



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