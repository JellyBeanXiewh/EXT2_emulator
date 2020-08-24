/*
 * @Author: Deng Cai 
 * @Date: 2019-09-09 16:09:14
 * @Last Modified by: Deng Cai
 * @Last Modified time: 2019-12-10 19:16:58
 */

//  in this code : \
    for the file system's space is so small, we don't need \
    group descriptors actually, so i just put \
    some critical information in other structures and cut group \
    descriptors. what's more, the inode map and block map are \
    put into super block.
//  in some degrees, this file system can be seemed as a system \
    with only one block group, so we need just one super block \
    and one group descriptor. then why don't we put them together \
    and make them one single structure as "super_block"? that's \
    how i handle with it.


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BLOCK_SIZE 1024
// 1KB.

typedef struct inode {
    // 32 bytes;
    uint32_t size;
    // the number of blocks it have.
    uint16_t file_type;
    // 1->dir; 0->file;
    uint16_t link;  // it doesn't matter if you \
        don't know what this variable means.
    uint32_t block_point[6];
    // the blocks belonging to this inode.
} inode;

typedef struct super_block {
    // 656 bytes;
    int32_t system_mod;
    // use system_mod to check if it \
        is the first time to run the FS.
    int32_t free_block_count;
    // 2^12; 4096;
    int32_t free_inode_count;
    // 1024;
    int32_t dir_inode_count;
    uint32_t block_map[128];
    // 512 bytes;
    uint32_t inode_map[32];
    // 128 bytes;
} sp_block;
// 1 block;

typedef struct dir_item {
    // the content of folders.
    // 128 bytes;
    uint32_t inode_id;
    uint16_t item_count;
    // 1 means the last one;
    // 2 means was deleted;
    // it doesn't matter if you don't understand it.
    uint8_t type;
    // 1 represents dir;
    char name[121];
} dir_item;

FILE *fp;
sp_block *spBlock;
inode inode_table[1024];
dir_item block_buffer[8];


void print_information();
// do some pre-work when you run the FS.
void fs_init();
void ls(char *path);
void create_file(char *path,int size);
void create_dir(char *path);
void delete_file(char *path);
void delete_dir(char *path);
void move(char *from,char *to);
void shutdown();

void print_help_info();