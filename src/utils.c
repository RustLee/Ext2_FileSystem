//
// Created by Rust Lee on 13/12/2020.
//

#include "../include/utils.h"

void buffer_read(const char *buf, void* dst, int buf_start, int size)
{
    char *dst_1 = (char*)dst;
    for (size_t i = 0; i < size; i++)
    {
        dst_1[i] = buf[i + buf_start];
    }
}

void buffer_write(char *buf,void *src,int buf_start,int size)
{
    char *temp = (char*)src;
    for (size_t i = 0; i < size; i++)
    {
        buf[i+buf_start] = temp[i];
    }
}

//字节位置1
void byte_set(uint32_t *bytes,int index)
{//从左往右，从小到大
    uint32_t temp = *bytes;
    temp |= (1<<(31-index));
    *bytes = temp;
}
//字节位置0
void byte_reset(uint32_t *bytes,int index)
{
    uint32_t temp = *bytes;
    temp &= ~(1<<(31-index));
    *bytes = temp;
}

void inode_map_set(int inode_index, int bit)
{
    int byte_index = inode_index / 32;
    int bit_index = inode_index % 32;
    if (bit == 1) {
        super_block.inode_map[byte_index] |= (uint32_t)(1 << (31 - bit_index));
    }
    else if (bit == 0) {
        super_block.inode_map[byte_index] &= (uint32_t)(~(1 << (31 - bit_index)));
    }
}

void block_map_set(int block_index, int bit)
{
    int byte_index = block_index / 32;
    int bit_index = block_index % 32;
    if (bit == 1) {
        super_block.block_map[byte_index] |= (uint32_t)(1 << (31 - bit_index));
    } else if (bit == 0) {
        super_block.block_map[byte_index] &= (uint32_t)(~(1 << (31 - bit_index)));
    }
}

int block_ergodic()
{
    uint32_t temp;
    for(int i = 0; i<128; i++) {
        temp = super_block.block_map[i];
        for(int j = 0; j<32; j++) {
            if(i == 0 && j == 0)
                j += 64;
            if ((temp & (1 << (31-j)))==0) {
                return i*128 + j;
            }
        }
    }
    return -1;
}

int inode_ergodic()
{
    uint32_t temp;
    for(int i = 0; i<32; i++) {
        temp = super_block.inode_map[i];
        for(int j = 0; j<32; j++) {
            if ((temp & (1 << (31 - j))) == 0) {
                return i*32 + j;
            }
        }
    }
    return 0;
}

void item_ergodic(dir_ent *dir,int *i,int *j, int *disk_blk_part,int type,int check_bit)
{
    dir_ent dir_ent_temp;
    dir_ent_temp.valid = 1;
    int block_point_id, disk_pos;
    int disk_part = 0;
    //在当前目录下找可以新建一个目录的位置
    for(block_point_id = 0; block_point_id < 6; block_point_id++) {
        int block_id = current_inode.block_point[block_point_id];
        if (block_id == 0)
            break;
        disk_read_block(block_id * 2, disk_rw_buf);
        for(disk_pos=0; disk_pos < 4; disk_pos++) {
            buffer_read(disk_rw_buf, &dir_ent_temp, disk_pos * DIR_SIZE, DIR_SIZE);
            //输入的文件在当前目录下存在同名，则创建失败
            if (strcmp(dir_ent_temp.name, path[0]) == 0 && type == dir_ent_temp.type
            && dir_ent_temp.valid == 1 && check_bit == 1) {
                dir->type = FILE_TYPE_ERR;
                return;
            }
            if (dir_ent_temp.valid == 0) {
                disk_part = 0;
                break;
            }
        }
        if (dir_ent_temp.valid == 0)
            break;
        disk_read_block(block_id * 2 + 1, disk_rw_buf);
        for(disk_pos = 0; disk_pos < 4; disk_pos++) {
            buffer_read(disk_rw_buf, &dir_ent_temp, disk_pos * DIR_SIZE, DIR_SIZE);
            if (strcmp(dir_ent_temp.name, path[0]) == 0 && type==dir_ent_temp.type
            && dir_ent_temp.valid == 1 && check_bit == 1) {
                dir->type = FILE_TYPE_ERR;
                return;
            }
            if (dir_ent_temp.valid == 0) {
                disk_part = 1;
                break;
            }
        }
        if (dir_ent_temp.valid == 0)
            break;
    }
    *dir = dir_ent_temp;
    *i = block_point_id;
    *j = disk_pos;
    *disk_blk_part = disk_part;
}

int file_ergodic(char file_name[FILE_NAME_LENGTH], int *block_point_index, int *item_index, int *disk_block, unsigned char type)
{
    dir_ent dir_ent_temp;
    dir_ent_temp.valid = 1;
    int temp_i, temp_j;
    for (temp_i = 0; temp_i < 6; temp_i++) {
        int block_id = current_inode.block_point[temp_i];
        if (block_id == 0)
            break;
        disk_read_block(block_id*2, disk_rw_buf);
        for (temp_j = 0; temp_j < 4; temp_j++) {
            buffer_read(disk_rw_buf, &dir_ent_temp, temp_j*DIR_SIZE, DIR_SIZE);
            if (strcmp(dir_ent_temp.name, file_name) == 0 && dir_ent_temp.valid == 1 && dir_ent_temp.type == type)
            {
                *block_point_index = temp_i;
                *item_index = temp_j;
                *disk_block = 0;
                return 1;
            }
        }
        disk_read_block(block_id * 2 + 1, disk_rw_buf);
        for(temp_j=0; temp_j < 4; temp_j++) {
            buffer_read(disk_rw_buf, &dir_ent_temp, temp_j*DIR_SIZE, DIR_SIZE);
            if (strcmp(dir_ent_temp.name, file_name) == 0 && dir_ent_temp.valid == 1 && dir_ent_temp.type == type) {
                *block_point_index = temp_i;
                *item_index = temp_j;
                *disk_block = 0;
                return 1;
            }
        }
    }
    return -1;
}

int path_process()
{
    int pos = 0;
    int level = 0;
    while (argv[pos] != '\0') {
        while (argv[pos] != '/' && argv[pos] != '\0') {
            path[level][pos] = argv[pos];
            pos++;
        }
        path[level][pos] = '\0';
        if(path[level][0] != '\0')
            level++;
    }
    for (size_t i = 0; i < FILE_NAME_LENGTH; i++) {
        path[level][i]=0;
    }
    return level;
}

char* token_process(char *buf, char *token)
{
    char *p = buf;
    while (*p==' ') {
        if (p < buf + BUF_SIZE)
            p++;
    }
    int flag = 0;
    while (*p != ' ' && *p != '\0') {
        token[flag] = *p;
        p++;
        flag++;
    }
    token[flag] = '\0';
    return p;
}