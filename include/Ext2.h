//
// Created by Rust Lee on 13/12/2020.
//

#ifndef FILE_SYSTEM_EXT2_H
#define FILE_SYSTEM_EXT2_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "disk.h"
#define BLOCK_SIZE 1024
#define MAX_BLOCK_NUM 4096
#define MAGIC_NUMBER 0x11223344
#define FILE_TYPE_FILE '1'
#define FILE_TYPE_DIR '2'
#define FILE_TYPE_ERR '3'
#define FILE_NAME_LENGTH 121
#define SUPER_BLOCK_INDEX 0
#define DISK_BLOCK_INDEX 0
#define INODE_INDEX 0
#define DIR_SIZE sizeof(dir_ent)
#define INODE_SIZE sizeof(index_node)
#define BUF_SIZE 256

char command_buf[1024];
char command[BUF_SIZE];
char argv[BUF_SIZE];
char disk_rw_buf[BLOCK_SIZE / 2];
char data_buf[BLOCK_SIZE];
char path[10][FILE_NAME_LENGTH];

typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;

typedef struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
} index_node;

typedef struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
} dir_ent;

sp_block super_block;
dir_ent dir_ent_root;

char current_path[10][FILE_NAME_LENGTH];
int current_level;
index_node current_inode;
uint32_t current_inode_index;

int ext2_init();
void super_block_read();
void super_block_ent_root_init();
index_node inode_init(unsigned char file_type);

void ls();
void mkdir();
void touch();
void cp();
void cd();
void shutdown();
#endif //FILE_SYSTEM_EXT2_H
