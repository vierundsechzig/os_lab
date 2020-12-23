#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <string.h>
#include "disk.h"

#define MAGIC (0x7fffffff)
#define FS_BLOCK_COUNT (1024)
#define FS_BLOCK_SIZE (4096)
#define TYPE_FILE (1)
#define TYPE_DIR (0)
#define N_DIRECT_PTR (7)
#define INODE_NUM (1024)
#define DATA_BEGIN (1 + INODE_NUM * (sizeof (struct inode)) / FS_BLOCK_SIZE)
#define INODE_PER_BLOCK (FS_BLOCK_SIZE / sizeof (struct inode))

extern const char* curdir;
extern const char* prtdir;
static char fs_buf[FS_BLOCK_SIZE];

// 超级块
struct superblock {
    int32_t magic;
    int32_t free_block_count;
    int32_t free_inode_count;
    int32_t dir_inode_count;
    uint8_t block_map[FS_BLOCK_COUNT / 8];
    uint8_t inode_map[INODE_NUM / 8];
};

// 索引节点
struct inode {
    uint32_t size : 22;
    uint32_t type : 2;
    uint32_t link : 8;
    uint32_t ptr[N_DIRECT_PTR];
};

// 目录项
struct dirent {
    uint16_t index : 13;
    uint16_t valid : 1;
    uint16_t type : 2;
    char name[126];
};

// 单个目录数据块
struct dirblk {
    struct dirent entries[FS_BLOCK_SIZE / sizeof (struct dirent)];
};

// 读取文件系统块
int fs_rd_block(unsigned int index, char* const fs_buf);

// 写入文件系统块
int fs_wr_block(unsigned int index, const char* const fs_buf);

// block_map置位
void bmap_set(unsigned int bit, struct superblock* ptr_spblock);

// block_map复位
void bmap_reset(unsigned int bit, struct superblock* ptr_spblock);

// block_map测试
int bmap_test(unsigned int bit, struct superblock* ptr_spblock);

// inode_map置位
void imap_set(unsigned int bit, struct superblock* ptr_spblock);

// inode_map复位
void imap_reset(unsigned int bit, struct superblock* ptr_spblock);

// inode_map复位
int imap_test(unsigned int bit, struct superblock* ptr_spblock);

// 寻找空余inode
int bmap_lookup(struct superblock* ptr_spblock);

// 寻找空余数据块
int imap_lookup(struct superblock* ptr_spblock);

// 文件系统是否存在
int exists();

// 文件系统格式化
int format();

// 读标号为id的inode
int rd_inode(int id, struct inode* dst);

// 写标号为id的inode
int wr_inode(int id, const struct inode* src);

// 在一个目录数据块中查找某文件名对应的目录索引，返回的是该索引在该块中的位置
int dirent_lookup(struct dirblk* e, const char* filename);

// 在一个目录数据块中查找空闲的目录项，返回的是该目录项在该块中的位置
int free_dirent_lookup(struct dirblk* e);

// 创建文件
int touch(int index_dir, const char* filename);

// 创建目录
int mkdir(int index_dir, const char* dirname);

// 读取字节
int readbyte(int index, int position);

// 在末尾追加字节
int appendbyte(int index, char byte);

// 写入字节
int writebyte(int index, int position, char byte);

// 获取文件inode序号
int openpath(const char* path);

// 复制文件内容
int clone(int src_inodeno, int dst_inodeno);

#endif
