#include <stdio.h>
#include "../include/Ext2.h"
#include "../include/utils.h"

int main() {
    ext2_init();
    while (true) {
        printf("==>");
        char *cmd_buf = command_buf;
        memset(cmd_buf, 0, sizeof(command_buf));
        gets(cmd_buf);
        //printf("%s\n", cmd_buf);
        memset(command, 0, BUF_SIZE);
        cmd_buf = token_process(cmd_buf, command);
        memset(argv, 0, BUF_SIZE);
        token_process(cmd_buf, argv);
        //printf("-----%s-----%s-----\n",command,argv);
        if (!strcmp(command, "ls")) {
            ls();
        }
        else if (!strcmp(command, "mkdir")) {
            mkdir();
        }
        else if (!strcmp(command, "touch")) {
            touch();
        }
        else if (!strcmp(command, "cp")) {
            cp();
        }
        else if (!strcmp(command, "cd")) {
            cd();
        }
        else if (!strcmp(command, "shutdown")) {
            shutdown();
            return 0;
        }
        else {
            printf("'%s' is not a command\n", command);
        }
    }
}
