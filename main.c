#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include "fs_operation.h"

//#define debug

int main() {
    char input_buffer[401];     // 输入缓冲区
    char *op = NULL;            // 记录指令操作符
    char *path = NULL;          // 记录路径
    char *arg = NULL;           // 记录参数
    char *errargs = NULL;       // 记录多余输入

    struct passwd *pwd = getpwuid(getuid());    // 获取用户信息
    char hostname[50];                          // 记录 hostname
    gethostname(hostname, 50);                  // 获取 hostname

    // 初始化
    spBlock = malloc(sizeof(sp_block));         // 初始化超级块
    fs_init();                           // 文件系统初始化

    // 输出若干信息
    printf("--------------------------------------------------------------------\n"
           "----------------------------- WELCOME! -----------------------------\n"
           "--------------------------------------------------------------------\n");
    print_information();

    while (1) {
        // 判断是否为 root 用户，如果是，提示符为 #，否则提示符为 $
        if (pwd->pw_uid == 0) {
            // \x1b[0m    : 默认
            // \x1b[1;32m : 高亮、绿色
            // \x1b[1;34m : 高亮、蓝色
            printf("\x1b[1;32m%s@%s\x1b[0m:\x1b[1;34m/\x1b[0m# ", pwd->pw_name, hostname);
        } else {
            printf("\x1b[1;32m%s@%s\x1b[0m:\x1b[1;34m/\x1b[0m$ ", pwd->pw_name, hostname);
        }

        strcpy(input_buffer, "");       // 清空
        scanf("%400[^\n]", input_buffer);   // 按格式读，每次读1行，最长为 399 个字符
        setbuf(stdin, NULL);        // 清空输入缓冲区

        op = strtok(input_buffer, " ");     // 分割命令

        if (op == NULL) {
            continue;
        } else if (strcmp(op, "ls") == 0) {         // ls
            path = strtok(NULL, " ");       // 分割路径
            errargs = strtok(NULL, " ");    // 其余输入

            // 错误处理
            if (path == NULL) {
                printf("ls: missing operand\n");
                continue;
            } else if (errargs != NULL) {
                printf("ls: invalid option -- \'%s\'\n", errargs);
                continue;
            }

            ls(path);
        } else if (strcmp(op, "create") == 0) {         // create
            arg = strtok(NULL, " ");        // 参数
            path = strtok(NULL, " ");       // 路径
            errargs = strtok(NULL, " ");    // 其余输入

            // 错误处理
            if (arg == NULL || path == NULL) {
                printf("create: missing operand\n");
                continue;
            } else if (errargs != NULL) {
                printf("create: invalid option --\'%s\'\n", errargs);
                continue;
            }

            int file_size = (int)strtol(arg, NULL, 10);     // 将参数转成数字，如果无法转换则得到 0

            if (strcmp(arg, "-d") == 0) {
                create_dir(path);
            } else if (file_size > 0) {
                create_file(path, file_size);
            } else {
                printf("create: invalid option -- \'%s\'\n", arg);
                continue;
            }
        } else if (strcmp(op, "delete") == 0) {     // delete
            arg = strtok(NULL, " ");
            path = strtok(NULL, " ");
            errargs = strtok(NULL, " ");

            if (arg == NULL || path == NULL) {
                printf("delete: missing operand\n");
                continue;
            } else if (errargs != NULL) {
                printf("delete: invalid option --\'%s\'\n", errargs);
                continue;
            }

            if (strcmp(arg, "-d") == 0) {
                delete_dir(path);
            } else if (strcmp(arg, "-f") == 0) {
                delete_file(path);
            } else {
                printf("delete: invalid option -- \'%s\'\n", arg);
                continue;
            }
        } else if (strcmp(op, "move") == 0) {           // move
            char *src = strtok(NULL, " ");
            char *dst = strtok(NULL, " ");
            errargs = strtok(NULL, " ");

            if (src == NULL || dst == NULL) {
                printf("move: missing operand\n");
                continue;
            } else if (errargs != NULL) {
                printf("move: invalid option --\'%s\'\n", errargs);
                continue;
            }

            move(src, dst);
        } else if (strcmp(op, "df") == 0) {         // 输出磁盘空间使用信息
            errargs = strtok(NULL, " ");

            if (errargs != NULL) {
                printf("shutdown: invalid option --\'%s\'\n", errargs);
                continue;
            }

            print_information();
        } else if (strcmp(op, "help") == 0) {
            errargs = strtok(NULL, " ");

            if (errargs != NULL) {
                printf("shutdown: invalid option --\'%s\'\n", errargs);
                continue;
            }

            print_help_info();
        } else if (strcmp(op, "shutdown") == 0) {   // shutdown
            errargs = strtok(NULL, " ");

            if (errargs != NULL) {
                printf("shutdown: invalid option --\'%s\'\n", errargs);
                continue;
            }

            shutdown();
            break;

// 仅在开发过程可用，用于格式化磁盘
#ifdef debug
        } else if (strcmp(op, "format") == 0) {     // format
            spBlock->system_mod = 0;
            fseek(fp, 0, SEEK_SET);
            fwrite(spBlock, sizeof(sp_block), 1, fp);
            fclose(fp);
            fs_init();
#endif
        } else {
            printf("%s: command not found\n", op);
        }
    }

    free(spBlock);  // 释放空间
    fclose(fp);

    return 0;
}
