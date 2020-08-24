#include "fs_operation.h"
#include <math.h>

#define SUPER_BLOCK_SIZE 1024
#define INODE_NUM 1024
#define SUPER_BLOCK_START 0
#define INODE_TABLE_START (SUPER_BLOCK_SIZE)

const char disk[] = "./disk.os";    // 磁盘文件

// 从磁盘加载超级块
void load_super_block() {
    fseek(fp, SUPER_BLOCK_START, SEEK_SET);
    fread(spBlock, sizeof(sp_block), 1, fp);
}

// 将超级块写入磁盘
void write_super_block() {
    fseek(fp, SUPER_BLOCK_START, SEEK_SET);
    fwrite(spBlock, sizeof(sp_block), 1, fp);
}

// 从磁盘加载索引表
void load_inode_table() {
    fseek(fp, INODE_TABLE_START, SEEK_SET);
    fread(inode_table, sizeof(inode), INODE_NUM, fp);
}

// 将索引表写入磁盘
void write_inode_table() {
    fseek(fp, INODE_TABLE_START, SEEK_SET);
    fwrite(inode_table, sizeof(inode), INODE_NUM, fp);
}

// 从磁盘加载数据块
void load_block(int32_t id) {
    fseek(fp, id * BLOCK_SIZE, SEEK_SET);
    fread(block_buffer, sizeof(dir_item), 8, fp);
}

// 将数据块写入磁盘
void write_block(int32_t id) {
    fseek(fp, id * BLOCK_SIZE, SEEK_SET);
    fwrite(block_buffer, sizeof(dir_item), 8, fp);
}

// 将 block 位图中对应位设为 1
void set_block_map_bit(int32_t id) {
    uint32_t index = id / 32;
    uint32_t bit = id % 32;
    uint32_t con = 0x1;
    con = con << bit;
    spBlock->block_map[index] |= con;
}

// 将 block 位图中对应位设为 0
void reset_block_map_bit(int32_t id) {
    uint32_t index = id / 32;
    uint32_t bit = id % 32;
    uint32_t con = 0x1;
    con = con << bit;
    spBlock->block_map[index] &= ~con;
}

// 将 inode 位图中对应位设为 1
void set_inode_map_bit(int32_t id) {
    uint32_t index = id / 32;
    uint32_t bit = id % 32;
    uint32_t con = 0x1;
    con = con << bit;
    spBlock->inode_map[index] |= con;
}

// 将 inode 位图中对应位设为 0
void reset_inode_map_bit(int32_t id) {
    uint32_t index = id / 32;
    uint32_t bit = id % 32;
    uint32_t con = 0x1;
    con = con << bit;
    spBlock->inode_map[index] &= ~con;
}

// 从 block 位图中找到一个空闲 block
int32_t get_free_block() {
    // 已满
    if (spBlock->free_block_count == 0) {
        return -1;
    }

    // 初始位置
    uint32_t index = 0;
    uint32_t bit;
    uint32_t con = 0x1;

    while (spBlock->block_map[index] == 0xFFFFFFFF) {
        index++;
    }

    for (bit = 0; bit < 32; bit++) {
        if (((spBlock->block_map[index] >> bit) & con) == 0) {
            return index * 32 + bit;
        }
    }

}

// 从 inode 位图中找到一个空闲 inode
int32_t get_free_inode() {
    // 已满
    if (spBlock->free_inode_count == 0) {
        return -1;
    }

    // 初始化
    uint32_t index = 0;
    uint32_t bit;
    uint32_t con = 0x1;

    while (spBlock->inode_map[index] == 0xFFFFFFFF) {
        index++;
    }

    for (bit = 0; bit < 32; bit++) {
        if (((spBlock->inode_map[index] >> bit) & con) == 0) {
            return index * 32 + bit;
        }
    }
}

// 从指定目录的 inode 中找到对应文件的 inode_id
int32_t find_inode_id(const char *file, inode *cur_inode) {
    for (int i = 0; i < cur_inode->size; i++) {
        load_block(cur_inode->block_point[i]);      // 加载 block
        for (int j = 0; j < 8; j++) {
            if (block_buffer[j].item_count == 2) {  // 已删除，跳过
                continue;
            }
            if (strcmp(block_buffer[j].name, file) == 0) {  // 找到文件，返回
                return block_buffer[j].inode_id;
            }
            if (block_buffer[j].item_count == 1) {  // 到达末尾，结束
                break;
            }
        }
    }
    return -1;  // 未找到，返回-1
}

// 分配一个 block
int32_t alloc_block() {
    // 已满
    if (spBlock->free_block_count == 0) {
        return -1;
    }

    int32_t block_id = get_free_block();    // 分配一个空闲 block
    spBlock->free_block_count--;            // 更新超级块信息
    set_block_map_bit(block_id);            // 标记为已分配
    write_super_block();             // 更新超级块信息到磁盘
    return block_id;
}

// 分配一个 inode
int32_t alloc_inode() {
    if (spBlock->free_inode_count == 0) {
        return -1;
    }
    int32_t inode_id = get_free_inode();    // 分配一个空闲 inode
    spBlock->free_inode_count--;            // 更新超级块信息
    set_inode_map_bit(inode_id);            // 标记为已分配
    write_super_block();             // 更新超级块信息到磁盘
    return inode_id;
}

// 释放一个 block
void free_block(int32_t block_id) {
    reset_block_map_bit(block_id);          // 标记为空闲
    spBlock->free_block_count++;            // 更新超级块信息
    write_super_block();
}

// 释放一个 inode
void free_inode(int32_t inode_id) {
    reset_inode_map_bit(inode_id);          // 标记为空闲
    spBlock->free_inode_count++;            // 更新超级块信息
    write_super_block();
}

// 根据路径获取对应文件的 inode_id
int32_t get_inode_id_by_path(char *path) {
    // 错误处理
    if (path == NULL) {
        return -1;
    }

    char *temp_path = malloc(sizeof(char) * strlen(path) + 1);  // 复制一份，strtok 会修改源字符串内容
    strcpy(temp_path, path);

    char *p = NULL;     // 指向当前解析的文件或目录名
    int32_t cur_inode_id = 0;   // 从根目录开始解析，根目录的 inode_id 为 0
    inode *cur_inode;
    p = strtok(temp_path, "/");
    while (p) {
        cur_inode = &inode_table[cur_inode_id];
        cur_inode_id = find_inode_id(p, cur_inode);
        if (cur_inode_id == -1) {
            break;
        }
        p = strtok(NULL, "/");      // update
    }
    free(temp_path);
    return cur_inode_id;
}

// 输出磁盘空间使用信息
void print_information() {
    int dir_num = spBlock->dir_inode_count;             // 文件夹数量
    int file_num = 1024 - spBlock->free_inode_count - spBlock->dir_inode_count;     // 文件数量
    int free_block_num = spBlock->free_block_count;     // 空闲 block 数量
    int free_inode_num = spBlock->free_inode_count;     // 空闲 inode 数量
    printf("In this FileSystem:\n"
           "The maximum size of a single file is 6KB;\n"
           "The maximum number of files and folders a single folder can contain is 46;\n"
           "The whole file system can contain mostly 1024 files and folders;\n"
           "**It has %d folders and %d files in this system now;\n"
           "**It has %dKB free space now;\n"
           "**And it can accept another %d now files or folders.\n"
           "--------------------------------------------------------------------\n"
           "!!!!!!! **The instruction should be shorter than 400 bytes** !!!!!!!\n"
           "--------------------------------------------------------------------\n",
           dir_num, file_num, free_block_num, free_inode_num);
}

// 文件系统初始化
void fs_init() {
    fp = fopen(disk, "r+b");    // 以读写二进制文件方式打开

    // 错误处理
    if (fp == NULL) {
        printf("Cannot open file \'%s\'", disk);
        exit(1);
    }

    load_super_block();          // 假设超级块已存在，加载超级块
    if (spBlock->system_mod == 1) {     // 非首次使用文件系统
        load_inode_table();             // 加载索引表
    } else {
        // init super block
        printf("File system does not exist.\nFormating...\n");

        memset(spBlock, 0, sizeof(sp_block));           // 初始化 super_block
        memset(inode_table, 0, sizeof(inode) * 1024);   // 初始化 inode_table

        // 文件系统每个块为 1KB，超级块大小为 656B，将其对齐到 1KB
        // 索引表占用 32B * 1024 = 32KB
        // 共占用 33 个 block

        // init block map
        spBlock->block_map[0] = 0xFFFFFFFF;             // super_block, inode_table
        spBlock->block_map[1] = 0x00000001;

        spBlock->free_block_count = 4096 - 1 - 32;      // super_block: 1, inode_table: 32 * 1024
        spBlock->free_inode_count = 1024;
        spBlock->dir_inode_count = 0;

        // 分配根目录
        int32_t inode_id = alloc_inode();       // 分配 inode

        inode_table[inode_id].size = 1;         // 1 个 block
        inode_table[inode_id].file_type = 1;    // 文件夹

        int32_t block_id = alloc_block();       // 分配 block
        inode_table[inode_id].block_point[0] = block_id;

        write_inode_table();                    // 更新 inode_table

        load_block(block_id);

        // 创建目录项 "."
        block_buffer[0].inode_id = 0;           // 根目录的 inode_id
        block_buffer[0].item_count = 0;
        block_buffer[0].type = 1;               // 文件夹
        strcpy(block_buffer[0].name, ".");

        // 创建目录项 ".."
        // 对于根目录，"." 和 ".." 均指向自身
        block_buffer[1].inode_id = 0;           // 根目录的 inode_id
        block_buffer[1].item_count = 1;         // 末尾
        block_buffer[1].type = 1;               // 文件夹
        strcpy(block_buffer[1].name, "..");

        write_block(block_id);

        spBlock->dir_inode_count++;             // 更新目录数
        spBlock->system_mod = 1;                // 标记为已格式化

        write_super_block();             // 更新超级块
    }
}

// ls
// path 指向文件目录时，输出该目录下的所有文件；指向文件时，输出该文件的文件名
void ls(char *path) {
    // 目录起始地址不是根目录
    if (path[0] != '/') {
        printf("ls: cannot access \'%s\': No such file or directory\n", path);
        return;
    }

    char *parent_path = malloc(sizeof(char) * strlen(path) + 1);    // 父目录
    int end = strlen(path);
    while (path[end] != '/') {
        end--;
    }
    strncpy(parent_path, path, end + 1);    // 设定父目录
    parent_path[end + 1] = '\0';

    // 查找父目录的 inode_id
    int32_t parent_inode_id = get_inode_id_by_path(parent_path);
    if (parent_inode_id == -1) {
        printf("ls: cannot access \'%s\': No such directory\n", path);
        free(parent_path);
        return;
    }

    // 父目录的 inode
    inode *parent_inode = &inode_table[parent_inode_id];
    if (parent_inode->file_type == 0) {
        printf("ls: cannot access \'%s\': Not a directory\n", path);
        free(parent_path);
        return;
    }

    // 获取目标路径
    int32_t cur_inode_id = get_inode_id_by_path(path);
    int32_t cur_block_id;
    if (cur_inode_id == -1) {
        printf("ls: cannot access \'%s\': No such file or directory\n", path);
        return;
    }

    inode *cur_inode;
    cur_inode = &inode_table[cur_inode_id];
    if (cur_inode->file_type == 0) {
        // 路径指向文件
        load_block(cur_inode->block_point[0]);
        printf("%s\n", block_buffer[0].name);
    } else {
        // 路径指向目录
        for (int i = 0; i < cur_inode->size; i++) {
            cur_block_id = cur_inode->block_point[i];
            load_block(cur_block_id);
            for (int j = 0; j < 8; j++) {
                if (block_buffer[j].item_count == 2) {  // 已删除
                    continue;
                }
                if (block_buffer[j].type == 1) {        // 文件夹
                    printf("*");
                }
                printf("%s  ", block_buffer[j].name);
                if (block_buffer[j].item_count == 1) {  // 末尾
                    break;
                }
            }
        }
        printf("\n");
    }
}

// 创建文件
void create_file(char *path, int size) {
    // 文件过大
    if (size > 6144) {
        printf("create: cannot create file \'%s\': file size should be between 0 and 6144\n", path);
        return;
    }

    // 起始路径非根目录
    if (path[0] != '/') {
        printf("create: cannot access \'%s\': No such directory\n", path);
        return;
    }

    char *parent_path = malloc(sizeof(char) * strlen(path) + 1);    // 父目录
    char name[121];
    int end = strlen(path);
    while (path[end] != '/') {
        end--;
    }
    int name_length = strlen(path) - end - 1;
    // 文件名过长
    if (name_length > 120) {
        printf("create: cannot create file \'%s\': file name cannot be longer than 120 Bytes\n", path);
        free(parent_path);
        return;
    }
    strncpy(name, path + end + 1, name_length + 1);
    strncpy(parent_path, path, end + 1);
    parent_path[end + 1] = '\0';

    // 父目录的 inode_id
    int32_t parent_inode_id = get_inode_id_by_path(parent_path);
    if (parent_inode_id == -1) {
        printf("create: cannot access \'%s\': No such directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 父目录的 inode
    inode *parent_inode = &inode_table[parent_inode_id];
    if (parent_inode->file_type == 0) {
        printf("create: cannot access \'%s\': Not a directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 查找是否存在同名文件或文件夹
    for (int i = 0; i < parent_inode->size; i++) {
        load_block(parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (block_buffer[j].item_count == 2) {
                continue;
            }
            if (strcmp(block_buffer[j].name, name) == 0) {
                printf("create: cannot create file \'%s\': File exists\n", path);
                free(parent_path);
                return;
            }
            if (block_buffer[j].item_count == 1) {
                break;
            }
        }
    }

    // 分配 inode
    int32_t inode_id = alloc_inode();
    if (inode_id == -1) {
        printf("create: cannot create file \'%s\': No enough space\n", path);
        free(parent_path);
        return;
    }

    inode *cur_inode = &inode_table[inode_id];

    // 根据 size 分配 block
    cur_inode->size = ceil(size / 1024.0);
    cur_inode->file_type = 0;
    for (int i = 0; i < cur_inode->size; i++) {
        int32_t block_id = alloc_block();
        if (block_id == -1) {
            // 空间不足，释放刚刚分配的 inode 和 block
            printf("create: cannot create file \'%s\': No enough space\n", path);
            for (int j = 0; j < i; j++) {
                free_block(cur_inode->block_point[j]);
            }
            free_inode(inode_id);
            free(parent_path);
            return;
        }
        cur_inode->block_point[i] = block_id;
    }
    write_inode_table();        // 更新索引表
    write_super_block(); // 更新超级块

    // 记录 dir_item
    load_block(cur_inode->block_point[0]);
    block_buffer[0].inode_id = inode_id;
    block_buffer[0].item_count = 1;         // 末尾
    block_buffer[0].type = 0;               // 文件
    strcpy(block_buffer[0].name, name);
    write_block(cur_inode->block_point[0]);

    // 更新父目录的 inode
    for (int i = 0; i < parent_inode->size; i++) {
        load_block(parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (block_buffer[j].item_count == 1) {
                // 找到末尾
                block_buffer[j].item_count = 0;     // 更新标记
                if (j < 7) {
                    block_buffer[j + 1].inode_id = inode_id;
                    block_buffer[j + 1].item_count = 1;     // 末尾
                    block_buffer[j + 1].type = 0;           // 文件
                    strcpy(block_buffer[j + 1].name, name);
                    write_block(parent_inode->block_point[i]);
                } else {
                    // 当前分配给父目录的 block 已满
                    // 已达上限，释放刚刚分配的 inode 和 block
                    if (parent_inode->size == 6) {
                        printf("create: cannot create file \'%s\': No enough space in directory\n", path);
                        block_buffer[j].item_count = 1;
                        for (int k = 0; k < cur_inode->size; k++) {
                            free_block(cur_inode->block_point[k]);
                        }
                        free_inode(inode_id);
                        free(parent_path);
                        return;
                    }
                    // 仍可分配
                    write_block(parent_inode->block_point[i]);
                    int block_id = alloc_block();       // 分配 block
                    parent_inode->block_point[i + 1] = block_id;
                    parent_inode->size++;

                    load_block(block_id);
                    block_buffer[0].inode_id = inode_id;
                    block_buffer[0].item_count = 1;     // 末尾
                    block_buffer[0].type = 0;           // 文件
                    strcpy(block_buffer[0].name, name);
                    write_block(parent_inode->block_point[i + 1]);
                    write_inode_table();
                }
                free(parent_path);
                return;
            } else if (block_buffer[j].item_count == 2) {
                // 找到已删除位
                block_buffer[j].inode_id = inode_id;
                block_buffer[j].item_count = 0;         // 标记为可用
                block_buffer[j].type = 0;               // 文件
                strcpy(block_buffer[j].name, name);
                write_block(parent_inode->block_point[i]);
                free(parent_path);
                return;
            }
        }
    }
}

// 创建文件夹
void create_dir(char *path) {
    // 错误处理
    if (path[0] != '/') {
        printf("create: cannot access \'%s\': No such directory\n", path);
        return;
    }

    // 删除末尾的 "/"
    if (path[strlen(path) - 1] == '/') {
        path[strlen(path) - 1] = '\0';
    }

    char *parent_path = malloc(sizeof(char) * strlen(path) + 1);
    char name[121];
    int end = strlen(path);
    while (path[end] != '/') {
        end--;
    }
    int name_length = strlen(path) - end - 1;
    // 文件夹名过长
    if (name_length > 120) {
        printf("create: cannot create directory \'%s\': file name cannot be longer than 120 Bytes\n", path);
        free(parent_path);
        return;
    }
    strncpy(name, path + end + 1, name_length + 1);
    strncpy(parent_path, path, end + 1);
    parent_path[end + 1] = '\0';

    // 父目录的 inode_id
    int32_t parent_inode_id = get_inode_id_by_path(parent_path);
    if (parent_inode_id == -1) {
        printf("create: cannot access \'%s\': No such directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 父目录的 inode
    inode *parent_inode = &inode_table[parent_inode_id];
    if (parent_inode->file_type == 0) {
        printf("create: cannot access \'%s\': Not a directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 查找是否存在同名文件或文件夹
    for (int i = 0; i < parent_inode->size; i++) {
        load_block(parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (block_buffer[j].item_count == 2) {
                continue;
            }
            if (strcmp(block_buffer[j].name, name) == 0) {
                printf("create: cannot create directory \'%s\': File exists\n", path);
                free(parent_path);
                return;
            }
            if (block_buffer[j].item_count == 1) {
                break;
            }
        }
    }

    // 分配 inode
    int32_t inode_id = alloc_inode();
    if (inode_id == -1) {
        printf("create: cannot create file \'%s\': No enough space\n", path);
        free(parent_path);
        return;
    }

    inode *cur_inode = &inode_table[inode_id];

    cur_inode->size = 1;        // 已分配 block 数量
    cur_inode->file_type = 1;   // 文件夹

    // 分配 block
    int32_t block_id = alloc_block();
    if (block_id == -1) {
        printf("create: cannot create file \'%s\': No enough space\n", path);
        free_inode(inode_id);
        free(parent_path);
        return;
    }

    cur_inode->block_point[0] = block_id;
    write_inode_table();

    load_block(cur_inode->block_point[0]);

    // 创建目录项 "."
    block_buffer[0].inode_id = inode_id;
    block_buffer[0].item_count = 0;
    block_buffer[0].type = 1;
    strcpy(block_buffer[0].name, ".");

    // 创建目录项 ".."
    block_buffer[1].inode_id = parent_inode_id;
    block_buffer[1].item_count = 1;
    block_buffer[1].type = 1;
    strcpy(block_buffer[1].name, "..");

    write_block(cur_inode->block_point[0]);

    spBlock->dir_inode_count++;
    write_super_block();

    // 更新父目录的 inode
    for (int i = 0; i < parent_inode->size; i++) {
        load_block(parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (block_buffer[j].item_count == 1) {
                // 末尾
                block_buffer[j].item_count = 0;
                if (j < 7) {
                    block_buffer[j + 1].inode_id = inode_id;
                    block_buffer[j + 1].item_count = 1;     // 末尾
                    block_buffer[j + 1].type = 1;           // 文件夹
                    strcpy(block_buffer[j + 1].name, name);
                    write_block(parent_inode->block_point[i]);
                } else {
                    // 当前分配给父目录的 block 已满
                    // 已达上限，释放刚刚分配的 inode 和 block
                    if (parent_inode->size == 6) {
                        printf("create: cannot create file \'%s\': No enough space in directory\n", path);
                        block_buffer[j].item_count = 1;
                        for (int k = 0; k < cur_inode->size; k++) {
                            free_block(cur_inode->block_point[k]);
                        }
                        free_inode(inode_id);
                        free(parent_path);
                        return;
                    }
                    // 未达到上限
                    write_block(parent_inode->block_point[i]);
                    int new_block_id = alloc_block();   // 分配 block
                    parent_inode->block_point[i + 1] = new_block_id;
                    parent_inode->size++;
                    load_block(new_block_id);
                    block_buffer[0].inode_id = inode_id;
                    block_buffer[0].item_count = 1;     // 末尾
                    block_buffer[0].type = 1;           // 文件夹
                    strcpy(block_buffer[0].name, name);
                    write_block(parent_inode->block_point[i + 1]);
                    write_inode_table();
                }
                free(parent_path);
                return;
            } else if (block_buffer[j].item_count == 2) {
                //已删除位
                block_buffer[j].inode_id = inode_id;
                block_buffer[j].item_count = 0;         // 标记为可用
                block_buffer[j].type = 1;               // 文件夹
                strcpy(block_buffer[j].name, name);
                write_block(parent_inode->block_point[i]);
                free(parent_path);
                return;
            }
        }
    }
}

// 删除文件
void delete_file(char *path) {
    // 错误处理
    if (path[0] != '/') {
        printf("delete: cannot access \'%s\': No such directory\n", path);
        return;
    }

    char *parent_path = malloc(sizeof(char) * strlen(path) + 1);
    char name[121];
    int end = strlen(path);
    while (path[end] != '/') {
        end--;
    }
    int name_length = strlen(path) - end - 1;
    // 不可能存在的文件名
    if (name_length > 120) {
        printf("delete: cannot access file \'%s\': file name cannot be longer than 120 Bytes\n", path);
        free(parent_path);
        return;
    }
    strncpy(name, path + end + 1, name_length + 1);
    strncpy(parent_path, path, end + 1);
    parent_path[end + 1] = '\0';

    // 父目录的 inode_id
    int32_t parent_inode_id = get_inode_id_by_path(parent_path);
    if (parent_inode_id == -1) {
        printf("delete: cannot access \'%s\': No such directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 父目录的 inode
    inode *parent_inode = &inode_table[parent_inode_id];
    if (parent_inode->file_type == 0) {
        printf("delete: cannot access \'%s\': Not a directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 目标文件的 inode_id
    int32_t cur_inode_id = get_inode_id_by_path(path);
    if (cur_inode_id == -1) {
        printf("delete: cannot access \'%s\': No such file or directory\n", path);
        free(parent_path);
        return;
    }

    inode *cur_inode = &inode_table[cur_inode_id];

    // 目标为文件夹
    if (cur_inode->file_type == 1) {
        printf("delete: cannot delete \'%s\': Is a directory\n", path);
        free(parent_path);
        return;
    }

    // 释放 block
    for (int i = 0; i < cur_inode->size; i++) {
        free_block(cur_inode->block_point[i]);
    }
    free_inode(cur_inode_id);   // 释放 inode

    // 更新父目录的 inode
    for (int i = 0; i < parent_inode->size; i++) {
        load_block(parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (strcmp(block_buffer[j].name, name) == 0) {
                if (block_buffer[j].item_count == 0) {
                    block_buffer[j].item_count = 2;     // 不是末尾，标记为已删除
                    write_block(parent_inode->block_point[i]);
                } else if (block_buffer[j].item_count == 1) {   // 末尾
                    // 寻找最后一个未删除文件
                    if (j == 0) {
                        j = 7;
                        free_block(parent_inode->block_point[i]);
                        parent_inode->size--;
                        i--;
                        load_block(parent_inode->block_point[i]);
                    } else {
                        j--;
                    }
                    while (block_buffer[j].item_count == 2) {
                        j--;
                        if (j < 0) {
                            free_block(parent_inode->block_point[i]);
                            parent_inode->size--;
                            i--;
                            load_block(parent_inode->block_point[i]);
                            j = 7;
                        }
                    }
                    block_buffer[j].item_count = 1;
                    write_block(parent_inode->block_point[i]);
                }
                free(parent_path);
                return;
            }
        }
    }
}

// 删除文件夹
void delete_dir(char *path) {
    // 错误处理
    if (path[0] != '/') {
        printf("delete: cannot access \'%s\': No such directory\n", path);
        return;
    }

    // 删除末尾的 "/"
    if (path[strlen(path) - 1] == '/') {
        path[strlen(path) - 1] = '\0';
    }

    char *parent_path = malloc(sizeof(char) * strlen(path) + 1);
    char name[121];
    int end = strlen(path);
    while (path[end] != '/') {
        end--;
    }
    int name_length = strlen(path) - end - 1;
    // 不合法的文件名
    if (name_length > 120) {
        printf("delete: cannot access directory \'%s\': file name cannot be longer than 120 Bytes\n", path);
        free(parent_path);
        return;
    }
    strncpy(name, path + end + 1, name_length + 1);
    strncpy(parent_path, path, end + 1);
    parent_path[end + 1] = '\0';

    // 跳过删除 "." 和 ".."
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("delete: refusing to delete \'.\' or \'..\' directory: skipping \'%s\'\n", path);
        free(parent_path);
        return;
    }

    // 父目录的 inode_id
    int32_t parent_inode_id = get_inode_id_by_path(parent_path);
    if (parent_inode_id == -1) {
        printf("delete: cannot access \'%s\': No such directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 父目录的 inode
    inode *parent_inode = &inode_table[parent_inode_id];
    if (parent_inode->file_type == 0) {
        printf("delete: cannot access \'%s\': Not a directory\n", parent_path);
        free(parent_path);
        return;
    }

    // 目标文件的 inode_id
    int32_t cur_inode_id = get_inode_id_by_path(path);
    if (cur_inode_id == -1) {
        printf("delete: cannot access \'%s\': No such file or directory\n", path);
        free(parent_path);
        return;
    }

    // 跳过删除 "/"
    if (cur_inode_id == 0) {
        printf("delete: refusing to delete \'/\': skipping \'%s\'\n", path);
        free(parent_path);
        return;
    }

    // 目标文件夹的 inode
    inode *cur_inode = &inode_table[cur_inode_id];

    // 目标文件不是文件夹
    if (cur_inode->file_type == 0) {
        printf("delete: cannot delete \'%s\': Is a file\n", path);
        free(parent_path);
        return;
    }

    // 删除文件夹下的文件和文件夹
    for (int i = 0; i < cur_inode->size; i++) {
        load_block(cur_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            // 跳过 "."、".." 和已删除
            if (strcmp(block_buffer[j].name, ".") != 0 && strcmp(block_buffer[j].name, "..") != 0 && block_buffer[j].item_count != 2) {
                char *sub_path = malloc(sizeof(char) * (strlen(path) + 121));   // 欲删除文件的完整路径
                strcpy(sub_path, path);
                if (sub_path[strlen(path) - 1] != '/') {
                    strcat(sub_path, "/");
                }
                strcat(sub_path, block_buffer[j].name);

                if (block_buffer[j].type == 1) {
                    // 删除文件夹
                    delete_dir(sub_path);
                } else {
                    // 删除文件
                    delete_file(sub_path);
                }
                load_block(cur_inode->block_point[i]);  // 重新加载
                free(sub_path);
            }
            if (block_buffer[j].item_count == 1) {  // 末尾
                break;
            }
        }
    }

    // 释放 block
    for (int i = 0; i < cur_inode->size; i++) {
        free_block(cur_inode->block_point[i]);
    }

    free_inode(cur_inode_id);                       // 释放 inode

    spBlock->dir_inode_count--;                     // 更新
    write_super_block();

    // 更新父目录的 inode
    for (int i = 0; i < parent_inode->size; i++) {
        load_block(parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (strcmp(block_buffer[j].name, name) == 0) {
                if (block_buffer[j].item_count == 0) {
                    // 未到末尾，标记为已删除
                    block_buffer[j].item_count = 2;
                    write_block(parent_inode->block_point[i]);
                } else if (block_buffer[j].item_count == 1) {
                    // 寻找最后一个未删除文件
                    j--;
                    while (block_buffer[j].item_count == 2) {
                        j--;
                        if (j < 0) {
                            free_block(parent_inode->block_point[i]);
                            parent_inode->size--;
                            i--;
                            load_block(parent_inode->block_point[i]);
                            j = 7;
                        }
                    }
                    block_buffer[j].item_count = 1;
                    write_block(parent_inode->block_point[i]);
                }
                free(parent_path);
                return;
            }
        }
    }
}

// 移动文件（不可移动文件夹）
void move(char *from, char *to) {
    // 错误处理
    if (from[0] != '/') {
        printf("move: cannot access \'%s\': No such directory\n", from);
        return;
    }
    if (to[0] != '/') {
        printf("move: cannot access \'%s\': No such directory\n", to);
        return;
    }

    char *from_parent_path = malloc(sizeof(char) * strlen(from) + 1);
    char name[121];
    int end = strlen(from);
    while (from[end] != '/') {
        end--;
    }
    int name_length = strlen(from) - end - 1;
    // 不合法文件名
    if (name_length > 120) {
        printf("move: cannot access directory \'%s\': file name cannot be longer than 120 Bytes\n", from);
        free(from_parent_path);
        return;
    }
    strncpy(name, from + end + 1, name_length + 1);
    strncpy(from_parent_path, from, end + 1);
    from_parent_path[end + 1] = '\0';

    // 源文件的父目录的 inode_id
    int32_t parent_inode_id = get_inode_id_by_path(from_parent_path);
    if (parent_inode_id == -1) {
        printf("move: cannot access \'%s\': No such directory\n", from_parent_path);
        free(from_parent_path);
        return;
    }

    // 源文件的父目录的 inode
    inode *from_parent_inode = &inode_table[parent_inode_id];
    if (from_parent_inode->file_type == 0) {
        printf("move: cannot access \'%s\': Not a directory\n", from_parent_path);
        free(from_parent_path);
        return;
    }

    // 源文件的 inode
    int32_t cur_inode_id = get_inode_id_by_path(from);
    if (cur_inode_id == -1) {
        printf("move: cannot access \'%s\': No such file or directory\n", from);
        free(from_parent_path);
        return;
    }

    inode *cur_inode = &inode_table[cur_inode_id];
    if (cur_inode->file_type == 1) {
        printf("move: cannot move file \'%s\': Is a directory\n", from);
        free(from_parent_path);
        return;
    }

    // 目标路径的 inode_id
    int32_t to_inode_id = get_inode_id_by_path(to);
    if (to_inode_id == -1) {
        printf("move: cannot access \'%s\': No such directory\n", to);
        free(from_parent_path);
        return;
    }

    // 目标路径的 inode
    inode *to_inode = &inode_table[to_inode_id];

    // 目标路径不是文件夹
    if (to_inode->file_type == 0) {
        printf("move: cannot access \'%s\': Not a directory\n", to);
        free(from_parent_path);
        return;
    }

    // 查找目标路径下是否存在同名文件或文件夹
    for (int i = 0; i < to_inode->size; i++) {
        load_block(to_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (block_buffer[j].item_count == 2) {
                continue;
            }
            if (strcmp(block_buffer[j].name, name) == 0) {
                printf("move: cannot move file \'%s\': File exists\n", from);
                free(from_parent_path);
                return;
            }
            if (block_buffer[j].item_count == 1) {
                break;
            }
        }
    }

    // 移动先更新目标路径的 inode，再更新源路径的 inode
    // 更新目标路径的 inode
    int finish = 0;     // 标志位，用于快速跳出多重循环
    for (int i = 0; i < to_inode->size && finish == 0; i++) {
        load_block(to_inode->block_point[i]);
        for (int j = 0; j < 8 && finish == 0; j++) {
            if (block_buffer[j].item_count == 1) {  // 找到末尾
                block_buffer[j].item_count = 0;
                if (j < 7) {
                    // 未满
                    block_buffer[j + 1].inode_id = cur_inode_id;
                    block_buffer[j + 1].item_count = 1;     // 末尾
                    block_buffer[j + 1].type = 0;           // 文件
                    strcpy(block_buffer[j + 1].name, name);
                    write_block(to_inode->block_point[i]);
                } else {
                    // 当前分配给目标路径的 block 已满
                    // 达到上限，取消移动
                    if (to_inode->size == 6) {
                        printf("move: cannot move file \'%s\': No enough space in directory\n", from);
                        block_buffer[j].item_count = 1;
                        free(from_parent_path);
                        return;
                    }
                    // 未达到上限
                    write_block(to_inode->block_point[i]);
                    int block_id = alloc_block();       // 分配 block
                    to_inode->block_point[i + 1] = block_id;
                    to_inode->size++;
                    load_block(block_id);
                    block_buffer[0].inode_id = cur_inode_id;
                    block_buffer[0].item_count = 1;     // 末尾
                    block_buffer[0].type = 0;           // 文件
                    strcpy(block_buffer[0].name, name);
                    write_block(to_inode->block_point[i + 1]);
                    write_inode_table();
                }
                finish = 1;
            } else if (block_buffer[j].item_count == 2) {
                // 已删除位
                block_buffer[j].inode_id = cur_inode_id;
                block_buffer[j].item_count = 0;
                block_buffer[j].type = 0;
                strcpy(block_buffer[j].name, name);
                write_block(to_inode->block_point[i]);
                finish = 1;
            }
        }
    }

    // 更新源路径的 inode
    for (int i = 0; i < from_parent_inode->size; i++) {
        load_block(from_parent_inode->block_point[i]);
        for (int j = 0; j < 8; j++) {
            if (strcmp(block_buffer[j].name, name) == 0) {
                if (block_buffer[j].item_count == 0) {  // 不是末尾，标记为已删除
                    block_buffer[j].item_count = 2;
                    write_block(from_parent_inode->block_point[i]);
                } else if (block_buffer[j].item_count == 1) {
                    // 寻找最后一个未删除文件
                    if (j == 0) {
                        j = 7;
                        free_block(from_parent_inode->block_point[i]);
                        from_parent_inode->size--;
                        i--;
                        load_block(from_parent_inode->block_point[i]);
                    } else {
                        j--;
                    }
                    while (block_buffer[j].item_count == 2) {
                        j--;
                        if (j < 0) {
                            free_block(from_parent_inode->block_point[i]);
                            from_parent_inode->size--;
                            i--;
                            load_block(from_parent_inode->block_point[i]);
                            j = 7;
                        }
                    }
                    block_buffer[j].item_count = 1;
                    write_block(from_parent_inode->block_point[i]);
                }
                free(from_parent_path);
                return;
            }
        }
    }
}

// 退出文件系统
void shutdown() {
    write_super_block();
    write_inode_table();
    printf("############################# GOODBYE! #############################\n");
    fclose(fp);
}

void print_help_info()
{
    printf("create:\n"
           "Usage: create SIZE FILE\n"
           "  or:  create OPTION DIRECTORY\n"
           "Create the FILE or DIRECTORY, if it does not already exist.\n"
           "  -d\tcreate directory\n\n"
           "delete:\n"
           "Usage: delete OPTION FILE\n"
           "Delete the FILE\n"
           "  -d\tdelete directory and its contents recursively.\n"
           "  -f\tdelete file\n\n"
           "df:\n"
           "Usage: df\n"
           "Show information about the file system.\n\n"
           "ls:\n"
           "Usage: ls FILE\n"
           "List information about the FILEs.\n\n"
           "move:\n"
           "Usage: move SOURCE DESTINATION\n"
           "move SOURCE to DESTINATION.\n\n"
           "shutdown:\n"
           "Usage: shutdown\n"
           "Shut down the file system.\n");
}
