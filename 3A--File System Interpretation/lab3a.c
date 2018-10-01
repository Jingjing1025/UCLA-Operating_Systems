// Name: Jingjing 
// Email: 
// ID:

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include "ext2_fs.h"

int stat;
int offset;

int TIMEBUFF = 50;
int imgFildes = 0;
int block_size = 0;
int num_inode = 0;
char fileType = '?';
char* imageFile = NULL;

struct ext2_super_block superBlock;
struct ext2_group_desc groupDesc;

void Error(char* errMsg)
{
    fprintf(stderr, "Error occurred when %s: %s\n", errMsg, strerror(errno));
    exit(1);
}

void super_block()
{
    offset = 1024;
    block_size = EXT2_MIN_BLOCK_SIZE << superBlock.s_log_block_size,

    stat = pread(imgFildes, &superBlock, sizeof(struct ext2_super_block), offset);
    if (stat < 0)
        Error("reading the super block");

    fprintf(stdout, "%s,%d,%d,%d,%d,%d,%d,%d\n",
            "SUPERBLOCK",
            superBlock.s_blocks_count,
            superBlock.s_inodes_count,
            block_size,
            superBlock.s_inode_size,
            superBlock.s_blocks_per_group,
            superBlock.s_inodes_per_group,
            superBlock.s_first_ino);
}

void group()
{
    offset = 1024 + block_size;
    //assume num_group = 1
    int num_group = 1;

    stat = pread(imgFildes, &groupDesc, num_group * sizeof(struct ext2_group_desc), offset);
    if (stat < 0)
        Error("reading the group descriptor");

    fprintf(stdout, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n",
            "GROUP",
            0,
            superBlock.s_blocks_count,
            superBlock.s_inodes_count,
            groupDesc.bg_free_blocks_count,
            groupDesc.bg_free_inodes_count,
            groupDesc.bg_block_bitmap,
            groupDesc.bg_inode_bitmap,
            groupDesc.bg_inode_table);
}

void free_block()
{
    // 1: used; 0: free
    __u8 *block = malloc(block_size);
    if (block == NULL)
        Error("allocating memory for block");

    offset = 1024+ (groupDesc.bg_block_bitmap - 1) * block_size;
    int size = superBlock.s_first_data_block;

    stat = pread(imgFildes, block, block_size, offset);
    if (stat < 0)
        Error("reading the free block");

    int i, j;
    for (i = 0; i < block_size; i++)
    {
        int mask = 1;
        for (j = 0; j < 8; j++)
        {
            stat = mask & block[i];
            if (stat == 0)
                fprintf(stdout, "BFREE,%d\n", size);
            mask <<= 1;
            size += 1;
        }
    }
    free(block);
}

void free_inode()
{
    __u8 *inode = malloc(block_size);
    if (inode == NULL)
        Error("allocating memory for free inodes");

    int offset = groupDesc.bg_inode_bitmap * block_size;
    int size = 1;

    stat = pread(imgFildes, inode, block_size, offset);
    if (stat < 0)
        Error("reading the free inode");

    int i, j;
    for (i = 0; i < block_size; i++)
    {
        int mask = 1;

        for (j = 0; j < 8; j++)
        {
            stat = mask & inode[i];
            if (stat == 0)
                fprintf(stdout, "IFREE,%d\n", size);
            mask <<= 1;
            size += 1;
        }
    }
    free(inode);
}

void direct (struct ext2_inode *inode, int num_inode)
{
    int i;
    for (i = 0; i < 12; i++)
    {
        char buff[block_size];
        struct ext2_dir_entry *dirEntry = (struct ext2_dir_entry*)buff;

        offset = block_size * inode->i_block[i];
        stat = pread(imgFildes, &buff, block_size, offset);
        if (stat < 0)
            Error("reading the direct inode");

        int size = 0;

        while (size < block_size && dirEntry->file_type != 0)
        {
            if (dirEntry != NULL)
            {
                int len = dirEntry->name_len;
                char name[len + 1];
                int i;
                for (i = 0; i < len; i++)
                    name[i] = dirEntry->name[i];
                name[len] = '\0';
                
                fprintf(stdout, "%s,%d,%d,%d,%d,%d,'%s'\n",
                        "DIRENT",
                        num_inode,
                        size,
                        dirEntry->inode,
                        dirEntry->rec_len,
                        dirEntry->name_len,
                        name);
            }
            size += dirEntry->rec_len;
            dirEntry = (void *)dirEntry + dirEntry->rec_len;
        }
    }
}

void indirect_1(int num_inode, int block, int a)
{
    int level = 1;

    unsigned int *indirectBlock = malloc(block_size);
    if (indirectBlock == NULL)
        Error("allocating memory for indirect blocks");

    unsigned int num = block_size/sizeof(unsigned int);

    offset = block_size * block;
    stat = pread(imgFildes, indirectBlock, block_size, offset);
    if(stat < 0)
        Error("reading the indirect inodes");

    unsigned int i;
    for (i = 0; i < num; i++)
    {
        if (indirectBlock[i] != 0)
        {
            if (a == 0)
                offset = i + 12;
            else if (a == 1)
                offset = i + 268;
            else if (a == 2)
                offset = i + 65804;
            fprintf(stdout, "%s,%d,%d,%d,%d,%d\n",
                    "INDIRECT",
                    num_inode,
                    level,
                    offset,
                    block,
                    indirectBlock[i]);
        }
    }
    free(indirectBlock);
}

void indirect_2(int num_inode, int block, int a)
{
    int level = 2;

    unsigned int *indirectBlock = malloc(block_size);
    if (indirectBlock == NULL)
        Error("allocating memory for indirect blocks");

    unsigned int num = block_size/sizeof(unsigned int);

    offset = block_size * block;
    stat = pread(imgFildes, indirectBlock, block_size, offset);
    if(stat < 0)
        Error("reading the indirect inodes");

    unsigned int i;
    for (i = 0; i < num; i++)
    {
        if (indirectBlock[i] != 0)
        {
            if (a == 0)
                offset = i + 268;
            else if (a == 1)
                offset = i + 65804;
            fprintf(stdout, "%s,%d,%d,%d,%d,%d\n",
                    "INDIRECT",
                    num_inode,
                    level,
                    offset,
                    block,
                    indirectBlock[i]);

            if (a == 0)
                indirect_1(num_inode, indirectBlock[i], 1);
            else if (a == 1)
                indirect_1(num_inode, indirectBlock[i], 2);
        }
    }
    free(indirectBlock);
}

void indirect_3(int num_inode, int block)
{
    int level = 3;

    unsigned int *indirectBlock = malloc(block_size);
    if (indirectBlock == NULL)
        Error("allocating memory for indirect blocks");

    unsigned int num = block_size/sizeof(unsigned int);

    offset = block_size * block;
    stat = pread(imgFildes, indirectBlock, block_size, offset);
    if(stat < 0)
        Error("reading the indirect inodes");

    unsigned int i;
    for (i = 0; i < num; i++)
    {
        if (indirectBlock[i] != 0)
        {
            offset = i + 65804;
            fprintf(stdout, "%s,%d,%d,%d,%d,%d\n",
                    "INDIRECT",
                    num_inode,
                    level,
                    offset,
                    block,
                    indirectBlock[i]);

            indirect_2(num_inode, indirectBlock[i], 1);
        }
    }
    free(indirectBlock);
}

void inode()
{
    int size = superBlock.s_inodes_per_group * superBlock.s_inode_size;
    struct ext2_inode *inodeTable = malloc(size);
    if (inodeTable == NULL)
        Error("allocating memory for inodes");

    offset = groupDesc.bg_inode_table * block_size;
    stat = pread(imgFildes, inodeTable, size, offset);
    if (stat < 0)
        Error("reading the inode");

    unsigned int i;
    int j;
    for (i = 0; i < superBlock.s_inodes_per_group; i++)
    {
        num_inode += 1;
        struct ext2_inode *inode = &inodeTable[i];

        if (inode->i_mode != 0 && inode->i_links_count != 0)
        {
            // regular file: 0x8000
            // directory: 0x4000
            // symbolic link: 0xA000
            if (inode->i_mode & 0x8000)
                fileType = 'f';
            else if (inode->i_mode & 0x4000)
                fileType = 'd';
            else if (inode->i_mode & 0xA000)
                fileType = 's';
            else
                fileType = '?';

            struct tm *info;

            char creat_time[TIMEBUFF];
            time_t ctime = inode->i_ctime;
            info = gmtime(&ctime);
            stat = strftime(creat_time, TIMEBUFF,  "%m/%d/%y %H:%M:%S", info);
            if (stat == 0)
                Error("formating the creation time");

            char modif_time[TIMEBUFF];
            time_t mtime = inode->i_mtime;
            info = gmtime(&mtime);
            stat = strftime(modif_time, TIMEBUFF,  "%m/%d/%y %H:%M:%S", info);
            if (stat == 0)
                Error("formating the modification time");

            char access_time[TIMEBUFF];
            time_t atime = inode->i_atime;
            info = gmtime(&atime);
            stat = strftime(access_time, TIMEBUFF,  "%m/%d/%y %H:%M:%S", info);
            if (stat == 0)
                Error("formating the access time");

            if (inode->i_block[12] != 0)
                indirect_1(num_inode, inode->i_block[12], 0);
            if (inode->i_block[13] != 0)
                indirect_2(num_inode, inode->i_block[13], 0);
            if (inode->i_block[14] != 0)
                indirect_3(num_inode, inode->i_block[14]);
            
            int mask = 0x0FFF;
            fprintf(stdout, "%s,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
                    "INODE",
                    num_inode,
                    fileType,
                    inode->i_mode & mask,
                    inode->i_uid,
                    inode->i_gid,
                    inode->i_links_count,
                    creat_time,
                    modif_time,
                    access_time,
                    inode->i_size,
                    inode->i_blocks);

            for (j = 0; j < 15; j++)
                fprintf(stdout, ",%d", inode->i_block[j]);

            fprintf(stdout, "\n");
            
            if (fileType == 'd')
                direct(inode, num_inode);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        Error("reading arguments");
    imageFile = argv[1];
    imgFildes = open(imageFile, O_RDONLY);
    if (imgFildes < 0)
        Error("opening the image file");

    super_block();
    group();
    free_block();
    free_inode();
    inode();

    return 0;
}
