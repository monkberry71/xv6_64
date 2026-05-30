#include "user.h"
#include "fcntl.h"
#include <stdint.h>

#define MAXARGS 10


enum cmd_type {
    CMD_EXEC = 1,
    CMD_REDIR,
    CMD_PIPE,
    CMD_LIST,
    CMD_BACK,
};

struct cmd {
    enum cmd_type type;
};

struct exec_cmd {
    enum cmd_type type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
};

struct redir_cmd {
    enum cmd_type type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
};

struct pipe_cmd {
    enum cmd_type type;
    struct cmd *left;
    struct cmd *right;
};

struct list_cmd {
    enum cmd_type type;
    struct cmd *left;
    struct cmd *right;
};

struct back_cmd {
    enum cmd_type type;
    struct cmd *cmd;
};

int64_t fork1(void);
void panic(char*);
struct cmd* parse_cmd(char*);

void run_cmd(struct cmd *cmd) {
    // nvr return
    int p[2];

    if(cmd == 0) exit();

    switch(cmd->type) {
        case CMD_EXEC:
            struct exec_cmd *ecmd = (void*) cmd;
            if(ecmd->argv[0] == 0) exit(); 
            exec(ecmd->argv[0], ecmd->argv);
            printf(2, "exec %s failed\n", ecmd->argv[0]);
            break;
        case CMD_REDIR:
            struct redir_cmd *rcmd = (void*) cmd;
            close(rcmd->fd); // to close stdout
            if(open(rcmd->file, rcmd->mode) < 0) { // get the lowest fd, which was held by stdout
                printf(2, "open %s failed\n", rcmd->file);
                exit();
            }
            run_cmd(rcmd->cmd);
            break;
        case CMD_LIST:
            struct list_cmd *lcmd = (void*) cmd;
            if(fork1() == 0) {
                run_cmd(lcmd->left);
            }
            wait();
            run_cmd(lcmd->right);
            break;
        case CMD_PIPE:
            struct pipe_cmd *pcmd = (void*) cmd;
            if(pipe(p) < 0) {
                panic("pipe failed");
            }

            if(fork1() == 0) {
                close(1); // close stdout
                dup(p[1]); // 1 becomes the pipe write file
                close(p[0]); // close pipe read fd
                close(p[1]); // close pipe write fd
                run_cmd(pcmd->left);
            }

            if(fork1() == 0) {
                close(0); // close stdin
                dup(p[0]); // 0 becomes the pipe read file
                close(p[0]);
                close(p[1]);
                run_cmd(pcmd->right);
            }

            close(p[0]);
            close(p[1]);
            wait();
            wait();
            break;
        case CMD_BACK:
            struct back_cmd *bcmd = (void*) cmd;
            if(fork1() == 0) {
                run_cmd(bcmd->cmd);
            }
            break;
        default:
            panic("run_cmd");
    }
    exit();
}

// fill buf with cmd
int get_cmd(char *buf, int nbuf) {
    printf(2, "$ "); // stdout may vary to any other file, so use stderr
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0) return -1;
    return 0;
} 

int main(void) {
    static char buf[128];

    // ensure three fd
    int fd;
    while((fd = open("/console", O_RDWR) >= 0)) {
        if(fd >= 3) {
            close(fd);
            break;
        }
    }

    while(get_cmd(buf, sizeof(buf)) >= 0) {
        if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') {
            buf[strlen(buf)-1] = 0;
            if(chdir(buf+3) < 0) {
                printf(2, "cannot cd %s", buf+3);
            }
            continue;
        }

        if(fork1() == 0) {
            run_cmd(parse_cmd(buf));
        }

        wait();
    }
    exit();
}

void panic(char *s) {
    printf(2, "%s\n", s);
    exit();
}


