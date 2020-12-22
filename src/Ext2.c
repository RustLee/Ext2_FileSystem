//
// Created by Rust Lee on 13/12/2020.
//

#include "../include/Ext2.h"
#include "../include/utils.h"

int ext2_init()
{
    if(open_disk()== -1){
        fprintf(stderr, "Disk open error\n");
        return -1;
    }

    if (disk_read_block(SUPER_BLOCK_INDEX ,disk_rw_buf)==-1) {
        fprintf(stderr,"Disk read error\n");
        return -1;
    }

    dir_ent_root.inode_id = INODE_INDEX;
    strcpy(dir_ent_root.name,"root");
    dir_ent_root.type = FILE_TYPE_DIR;
    dir_ent_root.valid = 1;
    super_block_read();
    int32_t magic_num = super_block.magic_num;

    //幻数为0，第一次进入系统，初始化超级块和根目录；不为0则直接获取根目录为当前目录
    if(magic_num == 0) {
        printf("Disk is already format.\nWelcome EXT2 file system...\n");
        super_block_ent_root_init();
    } else {
        printf("Welcome EXT2 file system...\n");
        struct inode inode_root;
        disk_read_block(2, disk_rw_buf);
        buffer_read(disk_rw_buf, &inode_root, INODE_INDEX, INODE_SIZE);
        current_inode = inode_root;
        current_inode_index = INODE_INDEX;
        current_level = 1;
        strcpy(current_path[0], "root");
    }
    return 0;
}

void super_block_read()
{
    char *sb = (char *) &super_block;
    int disk_rw_buf_idx = 0;

    memset(disk_rw_buf, 0, sizeof(disk_rw_buf));
    disk_read_block(SUPER_BLOCK_INDEX, disk_rw_buf);
    buffer_read(disk_rw_buf, sb, 0, 4*4);
    disk_rw_buf_idx += sizeof(uint32_t)*4;
    sb += (4+128)*sizeof(uint32_t);
    buffer_read(disk_rw_buf, sb, disk_rw_buf_idx, 32*sizeof(uint32_t));

    memset(disk_rw_buf, 0, sizeof(disk_rw_buf));
    disk_read_block(SUPER_BLOCK_INDEX + 1, disk_rw_buf);
    sb = (char *) &super_block;
    sb += 4*sizeof(uint32_t);
    disk_rw_buf_idx = 0;
    buffer_read(disk_rw_buf, sb, disk_rw_buf_idx, sizeof(uint32_t)*128);

}

void super_block_write()
{
    char *sb = (char *) &super_block;
    int disk_rw_buf_idx = 0;

    memset(disk_rw_buf, disk_rw_buf_idx, sizeof(disk_rw_buf));
    buffer_write(disk_rw_buf, sb, 0, 16);
    disk_rw_buf_idx += sizeof(uint32_t)*4;
    sb += (4 + 128) * sizeof(uint32_t);

    buffer_write(disk_rw_buf, sb, disk_rw_buf_idx, 32 * sizeof(uint32_t));
    disk_write_block(SUPER_BLOCK_INDEX, disk_rw_buf);

    sb = (char *) &super_block;
    sb += 4 * sizeof(uint32_t);
    disk_rw_buf_idx = 0;
    memset(disk_rw_buf, 0, sizeof(disk_rw_buf));
    buffer_write(disk_rw_buf, sb, disk_rw_buf_idx, sizeof(uint32_t) * 128);
    disk_write_block(SUPER_BLOCK_INDEX + 1, disk_rw_buf);
}

void super_block_ent_root_init()
{
    memset(disk_rw_buf, 0, sizeof(disk_rw_buf));
    index_node root_inode;
    root_inode = inode_init(FILE_TYPE_DIR);

    current_inode = root_inode;
    current_inode_index = 0;
    current_level = 1;
    strcpy(current_path[0], "root");

    buffer_write(disk_rw_buf, &root_inode, 0, INODE_SIZE);
    uint32_t disk_id = dir_ent_root.inode_id / 16 + 2;
    disk_write_block(disk_id, disk_rw_buf);
    memset(disk_rw_buf, 0, sizeof(disk_rw_buf));

    //写入新建的"."和".."目录信息
    dir_ent current_dir, pre_dir;

    current_dir.inode_id = dir_ent_root.inode_id;
    strcpy(current_dir.name, ".");
    current_dir.type = FILE_TYPE_DIR;
    current_dir.valid = 1;
    buffer_write(disk_rw_buf, &current_dir, 0, DIR_SIZE);

    pre_dir.inode_id = 0;
    strcpy(pre_dir.name, "..");
    pre_dir.type = FILE_TYPE_DIR;
    pre_dir.valid = 1;
    buffer_write(disk_rw_buf, &pre_dir, DIR_SIZE, DIR_SIZE);
    disk_write_block(128, disk_rw_buf);

    //初始化超级块, 0、1、64分别是超级块、inode数组、数据块的数据索引
    inode_map_set(dir_ent_root.inode_id,1);
    block_map_set(0,1);
    block_map_set(1, 1);
    block_map_set(64, 1);

    super_block.dir_inode_count = 1;
    super_block.magic_num = MAGIC_NUMBER;
    super_block.free_block_count = MAX_BLOCK_NUM - 3;
    super_block.free_inode_count = 1024 - 1;
    super_block_write();
    disk_read_block(128, disk_rw_buf);
}
//找到一个空闲的inode并初始化，在该目录下写入"."".."两个目录
index_node inode_init(unsigned char file_type)
{
    index_node inode;
    inode.file_type = FILE_TYPE_DIR;
    inode.link = 1;
    if (file_type == FILE_TYPE_DIR) {
        inode.size = DIR_SIZE*2;
        for (size_t i = 0; i < 6; i++) {
            inode.block_point[i] = 0;
        }

        int block_id = block_ergodic();
        block_map_set(block_id,1);
        if (block_id == -1) {
            inode.file_type = FILE_TYPE_ERR;
            return inode;
        }
        inode.block_point[0] = block_id;
        
        disk_read_block(block_id*2, disk_rw_buf);
        dir_ent current_dir, pre_dir;
        current_dir.inode_id = dir_ent_root.inode_id;
        strcpy(current_dir.name, ".");
        current_dir.type = FILE_TYPE_DIR;
        current_dir.valid = 1;
        buffer_write(disk_rw_buf, &current_dir, 0, DIR_SIZE);

        pre_dir.inode_id = 0;
        strcpy(pre_dir.name, "..");
        pre_dir.type = FILE_TYPE_DIR;
        pre_dir.valid = 1;
        buffer_write(disk_rw_buf, &pre_dir, DIR_SIZE, DIR_SIZE);
        disk_write_block(block_id*2, disk_rw_buf);
    } else if (file_type == FILE_TYPE_FILE) {
        inode.size = 0;
        for (size_t i = 0; i < 6; i++) {
            inode.block_point[i] = 0;
        }
        int block_id = block_ergodic();
        block_map_set(block_id, 1);
        if (block_id == -1) {
            inode.file_type = FILE_TYPE_ERR;
            return inode;
        }
        inode.block_point[0] = block_id;
    }
    return inode;

}

void ls() {
    index_node cur_inode = current_inode;
    int block_id;
    dir_ent dir_ent;
    char array[48][FILE_NAME_LENGTH] = {0};
    int dir_num = 0;
    int file_num = 0;
    for(int i = 0; i < 6; i++) {
        block_id = cur_inode.block_point[i];
        disk_read_block(block_id*2, disk_rw_buf);
        for(int j = 0; j < 4; j++) {
            buffer_read(disk_rw_buf, &dir_ent, j*DIR_SIZE, DIR_SIZE);
            if (dir_ent.valid==1) {
                if (dir_ent.type == FILE_TYPE_DIR) {
                    strcpy(array[dir_num],dir_ent.name);
                    dir_num++;
                } else if (dir_ent.type == FILE_TYPE_FILE) {
                    strcpy(array[47-file_num],dir_ent.name);
                    file_num++;
                }
            }
        }
        disk_read_block(block_id*2 + 1, disk_rw_buf);
        for(int j = 0;j < 4; j++) {
            buffer_read(disk_rw_buf, &dir_ent, j*DIR_SIZE, DIR_SIZE);
            if (dir_ent.valid == 1) {
                if (dir_ent.type == FILE_TYPE_DIR) {
                    strcpy(array[dir_num], dir_ent.name);
                    dir_num++;
                } else if (dir_ent.type == FILE_TYPE_FILE) {
                    strcpy(array[47-file_num], dir_ent.name);
                    file_num++;
                }
            }
        }
    }
    for(int i = 0; i < dir_num; i++) {
        printf("%s/ \n", array[i]);
    }
    for(int i=0; i < file_num; i++) {
        printf("%s \n", array[47-i]);
    }
}

void mkdir() {
    if (path_process() != 1) {
        fprintf(stderr, "PATH error!");
        return;
    }
    if (super_block.free_inode_count == 0 || super_block.free_block_count == 0) {
        fprintf(stderr,"No more space.\n");
        return;
    }
    dir_ent dir_ent;
    dir_ent.valid = 1;
    int block_point_id,disk_pos;
    int disk_blk_part;
    //在当前目录下找可以新建一个目录的位置
    item_ergodic(&dir_ent, &block_point_id, &disk_pos, &disk_blk_part, FILE_TYPE_DIR,1);
    if (dir_ent.type == FILE_TYPE_ERR) {
        fprintf(stderr,"Directory already exist.\n");
        return;
    }
    int free_inode = inode_ergodic();
    inode_map_set(free_inode, 1);
    //如果数据块和当前目录都已经用完，则退出
    if (block_point_id == 6 && dir_ent.valid == 1) {
        fprintf(stderr,"Current directory has no more space\n");
        return;
    }
    //数据块没用完但目录项不空闲，分配一个新的数据块
    else if (block_point_id < 6 && dir_ent.valid==1) {
        int block_id = block_ergodic();
        current_inode.block_point[block_point_id] = block_id;
        block_map_set(block_id, 1);
        super_block.free_block_count --;
        disk_read_block(block_id * 2, disk_rw_buf);
        super_block.free_inode_count --;
        inode_map_set(free_inode, 1);
        dir_ent.inode_id = free_inode;
        dir_ent.type = FILE_TYPE_DIR;
        strcpy(dir_ent.name, path[0]);

        buffer_write(disk_rw_buf, &dir_ent, 0, DIR_SIZE);
        disk_write_block(block_id * 2, disk_rw_buf);
    }
    //数据块没用完，目录项为空闲目录项
    else {
        int block_id = current_inode.block_point[block_point_id];
        super_block.free_inode_count--;
        inode_map_set(free_inode, 1);
        dir_ent.inode_id = free_inode;
        dir_ent.type = FILE_TYPE_DIR;
        strcpy(dir_ent.name, path[0]);
        dir_ent.valid = 1;
        buffer_write(disk_rw_buf, &dir_ent, disk_pos*DIR_SIZE, DIR_SIZE);
        disk_write_block(block_id*2 + disk_blk_part, disk_rw_buf);
    }

    //初始化新的inode
    index_node index_inode;
    index_inode = inode_init(FILE_TYPE_DIR);

    //写入新创建文件的inode信息
    int free_inode_id = free_inode / 16 + 2;
    int free_inode_index = free_inode % 16;
    disk_read_block(free_inode_id, disk_rw_buf);
    buffer_write(disk_rw_buf, &index_inode, free_inode_index*INODE_SIZE, INODE_SIZE);
    disk_write_block(free_inode_id, disk_rw_buf);
}

void touch()
{
    if (path_process() != 1) {
        fprintf(stderr, "PATH error!");
        return;
    }
    if (super_block.free_inode_count==0 || super_block.free_block_count==0) {
        printf("No more space\n");
        return;
    }
    dir_ent dir_ent;
    dir_ent.valid = 1;
    int block_point_id, disk_pos, disk_blk_part;
    item_ergodic(&dir_ent, &block_point_id, &disk_pos, &disk_blk_part, FILE_TYPE_FILE, 1);
    if (dir_ent.type==FILE_TYPE_ERR) {
        fprintf(stderr, "File has already exist.\n");
        return;
    }

    int free_inode = inode_ergodic();
    //实现基本同mkdir，不再赘述
    if (block_point_id == 6 && dir_ent.valid == 1) {
        fprintf(stderr, "Current directory has no more space\n");
        return;
    }
    else if (block_point_id < 6 && dir_ent.valid == 1)
    {
        int block_id = block_ergodic();
        current_inode.block_point[block_point_id] = block_id;
        block_map_set(block_id,1);
        super_block.free_block_count--;
        disk_read_block(block_id*2,disk_rw_buf);
        super_block.free_inode_count--;

        dir_ent.inode_id = free_inode;
        dir_ent.type = FILE_TYPE_FILE;
        strcpy(dir_ent.name,path[0]);
        buffer_write(disk_rw_buf, &dir_ent, 0, DIR_SIZE);
        disk_write_block(block_id*2,disk_rw_buf);
    }
    else {
        int block_id = current_inode.block_point[block_point_id];
        super_block.free_inode_count--;
        dir_ent.inode_id = free_inode;
        dir_ent.type = FILE_TYPE_FILE;
        strcpy(dir_ent.name,path[0]);
        dir_ent.valid = 1;
        buffer_write(disk_rw_buf, &dir_ent, disk_pos * DIR_SIZE, DIR_SIZE);
        disk_write_block(block_id*2 + disk_blk_part, disk_rw_buf);
    }

    //初始化新的inode
    index_node index_node;
    index_node = inode_init(FILE_TYPE_FILE);
    for(int k=1; k<6; k++)
    {
        index_node.block_point[k] = 0;
    }
    //写入新创建文件的inode信息
    int free_inode_id = free_inode / 16 + 2;
    int free_inode_index = free_inode % 16;
    disk_read_block(free_inode_id, disk_rw_buf);
    buffer_write(disk_rw_buf, &index_node, free_inode_index*INODE_SIZE, INODE_SIZE);
    disk_write_block(free_inode_id, disk_rw_buf);
}

void cp()
{
    if (path_process() != 1) {
        fprintf(stderr, "PATH error!");
        return;
    }
    if (super_block.free_inode_count==0 || super_block.free_block_count==0) {
        fprintf(stderr,"No more space.\n");
        return;
    }
    //检查当前目录下是否存在待复制的文件
    int block_point_index, item_index, disk_block;
    if(file_ergodic(path[0], &block_point_index, &item_index, &disk_block, FILE_TYPE_FILE) == -1) {
        fprintf(stderr,"No such file in current item.\n");
        return;
    }
    char new_name[FILE_NAME_LENGTH];
    int k;
    for(k=0; path[0][k]!='\0'; k++)
        new_name[k] = path[0][k];
    strcpy(new_name + k,"_1");

    dir_ent dir_ent;
    dir_ent.valid = 1;
    int block_point_id,disk_pos;
    int disk_blk_part;
    item_ergodic(&dir_ent, &block_point_id, &disk_pos, &disk_blk_part, FILE_TYPE_FILE,0);
    int free_inode = inode_ergodic();
    if (block_point_id==6 && dir_ent.valid==1) {
        fprintf(stderr, "Current directory has no more space.\n");
        return;
    }
    else if (block_point_id<6 && dir_ent.valid==1) {
        int block_id = block_ergodic();
        current_inode.block_point[block_point_id] = block_id;
        block_map_set(block_id,1);
        super_block.free_block_count--;
        disk_read_block(block_id*2, disk_rw_buf);
        super_block.free_inode_count--;

        dir_ent.inode_id = free_inode;
        dir_ent.type = FILE_TYPE_FILE;
        strcpy(dir_ent.name,new_name);
        buffer_write(disk_rw_buf, &dir_ent,0,DIR_SIZE);
        disk_write_block(block_id*2,disk_rw_buf);
    }
    else {
        int block_id = current_inode.block_point[block_point_id];
        super_block.free_inode_count--;
        dir_ent.inode_id = free_inode;
        dir_ent.type = FILE_TYPE_FILE;
        strcpy(dir_ent.name,new_name);
        dir_ent.valid = 1;
        buffer_write(disk_rw_buf, &dir_ent, disk_pos*DIR_SIZE, DIR_SIZE);
        disk_write_block(block_id*2 + disk_blk_part, disk_rw_buf);
    }

    index_node index_node;
    index_node = inode_init(FILE_TYPE_FILE);
    for(int i=1; i < 6; i++)
    {
        index_node.block_point[i] = 0;
    }

    int free_inode_id = free_inode / 16 + 2;
    int free_inode_index = free_inode % 16;
    disk_read_block(free_inode_id, disk_rw_buf);
    buffer_write(disk_rw_buf, &index_node, free_inode_index*INODE_SIZE, INODE_SIZE);
    disk_write_block(free_inode_id, disk_rw_buf);

}

void shutdown()
{
    super_block_write();
    printf("Exit EXT2 file system, thanks for using.\n");
}


void cd()
{
    if (path_process() != 1) {
        fprintf(stderr, "PATH error!");
        return;
    }
    int block_point_idx,item_index,disk_blk_part;
    int flag=0;
    if (!strcmp(path[0],".."))
    {
        flag = 0;
        if (!strcmp(current_path[current_level-1], "root")) {
            fprintf(stderr,"Can't cd to directory above root!\n");
            return;
        }
        else {
            file_ergodic(path[0], &block_point_idx, &item_index, &disk_blk_part, FILE_TYPE_DIR);
        }
    }
    else {
        flag = 1;
        if(file_ergodic(path[0], &block_point_idx, &item_index, &disk_blk_part, FILE_TYPE_DIR)==-1) {
            fprintf(stderr,"No such directory in current item.\n");
            return;
        }
    }

    dir_ent dir_ent;
    disk_read_block(current_inode.block_point[block_point_idx]*2 + disk_blk_part, disk_rw_buf);
    buffer_read(disk_rw_buf, &dir_ent, item_index*DIR_SIZE, DIR_SIZE);

    uint32_t block_inode_id = (dir_ent.inode_id) / 16 + 2;
    uint32_t block_inode_index = (dir_ent.inode_id) % 16;
    disk_read_block(block_inode_id,disk_rw_buf);
    struct inode dst_inode;
    buffer_read(disk_rw_buf,&dst_inode,block_inode_index*INODE_SIZE,INODE_SIZE);

    current_inode = dst_inode;
    for(int i = 0; i<6; i++)
        current_inode.block_point[i] = dst_inode.block_point[i];

    current_inode.file_type = dst_inode.file_type;
    current_inode.link = dst_inode.link;
    current_inode.size = dst_inode.size;
    current_inode_index = block_inode_index;

    if (flag) {
        strcpy(current_path[current_level],dir_ent.name);
        current_level++;
    }
    else
        current_level--;

    for(int i = 0; i < current_level; i++) {
        printf("%s/",current_path[i]);
    }

}