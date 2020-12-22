//
// Created by Rust Lee on 13/12/2020.
//

#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include "Ext2.h"

void buffer_read(const char *buf,void* dst,int buf_start,int size);
void buffer_write(char *buf,void *src,int buf_start,int size);
void inode_map_set(int inode_index, int bit);
void block_map_set(int block_index, int bit);
int file_ergodic(char file_name[FILE_NAME_LENGTH],
        int *block_point_index, int *item_index, int *disk_block, unsigned char type);
int block_ergodic();
int inode_ergodic();
void item_ergodic(dir_ent *dir,int *i,int *j, int *disk_blk_part,int type,int check_bit);
int path_process();
char* token_process(char *buf, char *token);
#endif //FILE_SYSTEM_UTILS_H
