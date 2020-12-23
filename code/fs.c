#include "fs.h"

#include <stdio.h>

const char* curdir = ".";
const char* prtdir = "..";

int fs_rd_block(unsigned int index, char* const fs_buf)
{
    if (index >= FS_BLOCK_COUNT)
        return -1;
    char disk_buf[DEVICE_BLOCK_SIZE];
    char* p = fs_buf;
    if (open_disk() == -1)
        return -1;
    for (int i=0; i<FS_BLOCK_SIZE/DEVICE_BLOCK_SIZE; ++i)
    {
        if (disk_read_block(index * (FS_BLOCK_SIZE / DEVICE_BLOCK_SIZE) + i, disk_buf) == -1)
        {
            close_disk();
            return -1;
        }
        memcpy(p, disk_buf, DEVICE_BLOCK_SIZE);
        p += DEVICE_BLOCK_SIZE;
    }
    close_disk();
    return 0;
}

int fs_wr_block(unsigned int index, const char* const fs_buf)
{
    if (index >= FS_BLOCK_COUNT)
        return -1;
    char disk_buf[DEVICE_BLOCK_SIZE];
    const char* p = fs_buf;
    if (open_disk() == -1)
        return -1;
    for (int i=0; i<FS_BLOCK_SIZE/DEVICE_BLOCK_SIZE; ++i)
    {
        memcpy(disk_buf, p, DEVICE_BLOCK_SIZE);
        if (disk_write_block(index * (FS_BLOCK_SIZE / DEVICE_BLOCK_SIZE) + i, disk_buf) == -1)
            return -1;
        p += DEVICE_BLOCK_SIZE;
    }
    close_disk();
    return 0;
}

void bmap_set(unsigned int bit, struct superblock* ptr_spblock)
{
    unsigned int array_index = bit >> 3;
    unsigned int op_bit = bit % 8;
    unsigned char mask = 1 << op_bit;
    ptr_spblock->block_map[array_index] |= mask;
}

void bmap_reset(unsigned int bit, struct superblock* ptr_spblock)
{
    unsigned int array_index = bit >> 3;
    unsigned int op_bit = bit % 8;
    unsigned char mask = ~(1 << op_bit);
    ptr_spblock->block_map[array_index] &= mask;
}

int bmap_test(unsigned int bit, struct superblock* ptr_spblock)
{
    unsigned int array_index = bit >> 3;
    unsigned int op_bit = bit % 8;
    return (ptr_spblock->block_map[array_index] >> op_bit) & 1;
}

void imap_set(unsigned int bit, struct superblock* ptr_spblock)
{
    unsigned int array_index = bit >> 3;
    unsigned int op_bit = bit % 8;
    unsigned char mask = 1 << op_bit;
    ptr_spblock->inode_map[array_index] |= mask;
}

void imap_reset(unsigned int bit, struct superblock* ptr_spblock)
{
    unsigned int array_index = bit >> 3;
    unsigned int op_bit = bit % 8;
    unsigned char mask = ~(1 << op_bit);
    ptr_spblock->inode_map[array_index] &= mask;
}

int imap_test(unsigned int bit, struct superblock* ptr_spblock)
{
    unsigned int array_index = bit >> 3;
    unsigned int op_bit = bit % 8;
    return (ptr_spblock->inode_map[array_index] >> op_bit) & 1;
}

int bmap_lookup(struct superblock* ptr_spblock)
{
    for (int i=0; i<FS_BLOCK_COUNT; ++i)
        if (bmap_test(i, ptr_spblock) == 0)
            return i;
    return -1;
}

int imap_lookup(struct superblock* ptr_spblock)
{
    for (int i=0; i<INODE_NUM; ++i)
        if (imap_test(i, ptr_spblock) == 0)
            return i;
    return -1;
}

int exists()
{
    struct superblock spblock;
    fs_rd_block(0, fs_buf);
    memcpy(&spblock, fs_buf, sizeof (struct superblock));
    return spblock.magic == MAGIC;
}

int format()
{
    static struct superblock spblock = {
        .magic = MAGIC,
        .free_block_count = FS_BLOCK_COUNT - DATA_BEGIN - 1,
        .free_inode_count = INODE_NUM - 1,
        .dir_inode_count = 1,
        .block_map = {0},
        .inode_map = {0}
    };
    static struct inode inode_root_dir = {
        .size = 2 * sizeof (struct dirent),
        .type = TYPE_DIR,
        .link = 1,
        .ptr = {0}
    };
    static struct dirblk blk_root_dir = {0};
    // "."
    blk_root_dir.entries[0].index = 0;
    blk_root_dir.entries[0].valid = 1;
    blk_root_dir.entries[0].type = TYPE_DIR;
    memcpy(blk_root_dir.entries[0].name, curdir, 2);
    // ".."
    blk_root_dir.entries[1].index = 0;
    blk_root_dir.entries[1].valid = 1;
    blk_root_dir.entries[1].type = TYPE_DIR;
    memcpy(blk_root_dir.entries[1].name, prtdir, 3);
    // commit root directory
    memset(fs_buf, 0, FS_BLOCK_SIZE);
    memcpy(fs_buf, &blk_root_dir, sizeof (struct dirblk));
    if (fs_wr_block(DATA_BEGIN, fs_buf) == -1)
        return -1;
    // commit superblock
    bmap_set(0, &spblock);
    imap_set(0, &spblock);
    memset(fs_buf, 0, FS_BLOCK_SIZE);
    memcpy(fs_buf, &spblock, sizeof (struct superblock));    
    if (fs_wr_block(0, fs_buf) == -1)
        return -1;
    // commit inode
    memset(fs_buf, 0, FS_BLOCK_SIZE);
    memcpy(fs_buf, &inode_root_dir, sizeof (struct inode));
    if (fs_wr_block(1, fs_buf) == -1)
        return -1;
    return 0;
}

int rd_inode(int id, struct inode* dst)
{
    int fs_block = id / (FS_BLOCK_SIZE / sizeof (struct inode)) + 1;
    int offset = id % (FS_BLOCK_SIZE / sizeof (struct inode)) * sizeof (struct inode);

    if (fs_rd_block(fs_block, fs_buf) == -1)
        return -1;
    memcpy(dst, fs_buf + offset, sizeof (struct inode));
    return 0;
}

int wr_inode(int id, const struct inode* src)
{
    int fs_block = id / (FS_BLOCK_SIZE / sizeof (struct inode)) + 1;
    int offset = id % (FS_BLOCK_SIZE / sizeof (struct inode)) * sizeof (struct inode);

    if (fs_rd_block(fs_block, fs_buf) == -1)
        return -1;
    memcpy(fs_buf + offset, src, sizeof (struct inode));
    if (fs_wr_block(fs_block, fs_buf) == -1)
        return -1;
    return 0;
}

int dirent_lookup(struct dirblk* e, const char* filename)
{
    int i;
    for (i=0; i<FS_BLOCK_SIZE / sizeof (struct dirent); ++i)
        if (e->entries[i].valid && strcmp(e->entries[i].name, filename) == 0)
            return i;
    return -1;
}

int free_dirent_lookup(struct dirblk* e)
{
    int i;
    for (i=0; i<FS_BLOCK_SIZE / sizeof (struct dirent); ++i)
        if (!e->entries[i].valid)
            return i;
    return -1;
}

int touch(int index_dir, const char* filename)
{
    if (strcmp(filename, curdir) == 0 || strcmp(filename, prtdir) == 0) // filename cannot be "." or ".."
        return -1;
    struct inode inode_buf, file_inode;
    static struct dirblk dir_buf;
    int32_t size;
    int index;
    struct superblock spblock;
    int bmap_index, imap_index;
    int dir_bmap_index;
    if (rd_inode(index_dir, &inode_buf) == -1)
        return -1;
    size = inode_buf.size;
    int i = 0;
    while (size > 0)
    {
        if (fs_rd_block(DATA_BEGIN + inode_buf.ptr[i], fs_buf) == -1)
            return -1;
        memcpy(&dir_buf, fs_buf, FS_BLOCK_SIZE);
        if ((index = dirent_lookup(&dir_buf, filename)) >= 0)
            return -1;
        ++i;
        size -= FS_BLOCK_SIZE;
    }
    size = inode_buf.size;
    i = 0;
    while (size > 0)
    {
        if (fs_rd_block(DATA_BEGIN + inode_buf.ptr[i], fs_buf) == -1)
            return -1;
        memcpy(&dir_buf, fs_buf, FS_BLOCK_SIZE);
        if ((index = free_dirent_lookup(&dir_buf)) >= 0)
        {
            // read-modify-write superblock
            if (fs_rd_block(0, fs_buf) == -1)
                return -1;
            memcpy(&spblock, fs_buf,  sizeof (struct superblock));
            if ((bmap_index = bmap_lookup(&spblock)) == -1)
                return -1;
            if ((imap_index = imap_lookup(&spblock)) == -1)
                return -1;
            bmap_set(bmap_index, &spblock);
            imap_set(imap_index, &spblock);
            spblock.free_block_count -= 1;
            spblock.free_inode_count -= 1;
            // update directory entry
            dir_buf.entries[index].index = imap_index;
            dir_buf.entries[index].type = TYPE_FILE;
            dir_buf.entries[index].valid = 1;
            strcpy(dir_buf.entries[index].name, filename);
            // update directory inode
            inode_buf.size += sizeof (struct dirent);
            // create file inode
            file_inode.size = 0;
            file_inode.type = TYPE_FILE;
            file_inode.link = 1;
            file_inode.ptr[0] = bmap_index;

            // initialize data block
            memset(fs_buf, 0, FS_BLOCK_SIZE);
            if (fs_wr_block(DATA_BEGIN + bmap_index, fs_buf) == -1)
                return -1;
            // commit superblock changes
            memcpy(fs_buf, &spblock,  sizeof (struct superblock));
            if (fs_wr_block(0, fs_buf) == -1)
                return -1;
            // commit file inode changes
            if (wr_inode(imap_index, &file_inode) == -1)
                return -1;
            // commit directory entry changes
            memcpy(fs_buf, &dir_buf, FS_BLOCK_SIZE);
            if (fs_wr_block(DATA_BEGIN + inode_buf.ptr[i], fs_buf) == -1)
                return -1;
            // commit directory inode changes
            if (wr_inode(index_dir, &inode_buf) == -1)
                return -1;
            return imap_index;
        }
        ++i;
        size -= FS_BLOCK_SIZE;
    }
    // allocate a new data block for directory entry
    if (i < N_DIRECT_PTR)
    {
        // read-modify superblock
        if (fs_rd_block(0, fs_buf) == -1)
            return -1;
        memcpy(&spblock, fs_buf,  sizeof (struct superblock));
        if ((dir_bmap_index = bmap_lookup(&spblock)) == -1)
            return -1;
        bmap_set(dir_bmap_index, &spblock);
        if ((bmap_index = bmap_lookup(&spblock)) == -1)
            return -1;
        if ((imap_index = imap_lookup(&spblock)) == -1)
            return -1;
        bmap_set(bmap_index, &spblock);
        imap_set(imap_index, &spblock);
        spblock.free_block_count -= 2;
        spblock.free_inode_count -= 1;
        // initialize new directory block
        memset(&dir_buf, 0, sizeof (struct dirblk));
        dir_buf.entries[0].index = imap_index;
        dir_buf.entries[0].type = TYPE_FILE;
        dir_buf.entries[0].valid = 1;
        strcpy(dir_buf.entries[0].name, filename);
        // update directory inode
        inode_buf.size += sizeof (struct dirent);
        inode_buf.ptr[i] = dir_bmap_index;
        // create file inode
        file_inode.size = 0;
        file_inode.type = TYPE_FILE;
        file_inode.link = 1;
        file_inode.ptr[0] = bmap_index;

        // initialize data block
        memset(fs_buf, 0, FS_BLOCK_SIZE);
        if (fs_wr_block(DATA_BEGIN + bmap_index, fs_buf) == -1)
            return -1;
        // commit new directory block
        memcpy(fs_buf, &dir_buf, FS_BLOCK_SIZE);
        if (fs_wr_block(DATA_BEGIN + dir_bmap_index, fs_buf) == -1)
            return -1;
        // commit superblock changes
        memcpy(fs_buf, &spblock,  sizeof (struct superblock));
        if (fs_wr_block(0, fs_buf) == -1)
            return -1;
        // commit file inode
        if (wr_inode(imap_index, &file_inode) == -1)
            return -1;
        // commit directory inode
        if (wr_inode(index_dir, &inode_buf) == -1)
            return -1;
        return imap_index;
    }
    return -1;
}

// 创建目录
int mkdir(int index_dir, const char* dirname)
{
    struct inode inode_dir, inode_chddir;
    static struct dirblk dir_buf, chddir_buf;
    int32_t size;
    int index;
    struct superblock spblock;
    int bmap_index, imap_index;
    int prtdir_bmap_index;
    if (rd_inode(index_dir, &inode_dir) == -1)
        return -1;
    size = inode_dir.size;
    int i = 0;
    while (size > 0)
    {
        if (fs_rd_block(DATA_BEGIN + inode_dir.ptr[i], fs_buf) == -1)
            return -1;
        memcpy(&dir_buf, fs_buf, FS_BLOCK_SIZE);
        if ((index = dirent_lookup(&dir_buf, dirname)) >= 0)
            return -1;
        ++i;
        size -= FS_BLOCK_SIZE;
    }
    size = inode_dir.size;
    i = 0;
    while (size > 0)
    {
        if (fs_rd_block(DATA_BEGIN + inode_dir.ptr[i], fs_buf) == -1)
            return -1;
        memcpy(&dir_buf, fs_buf, FS_BLOCK_SIZE);
        if ((index = free_dirent_lookup(&dir_buf)) >= 0)
        {
            // read-modify-write superblock
            if (fs_rd_block(0, fs_buf) == -1)
                return -1;
            memcpy(&spblock, fs_buf,  sizeof (struct superblock));
            if ((bmap_index = bmap_lookup(&spblock)) == -1)
                return -1;
            if ((imap_index = imap_lookup(&spblock)) == -1)
                return -1;
            bmap_set(bmap_index, &spblock);
            imap_set(imap_index, &spblock);
            spblock.free_block_count -= 1;
            spblock.free_inode_count -= 1;
            spblock.dir_inode_count += 1;
            // update directory entry
            dir_buf.entries[index].index = imap_index;
            dir_buf.entries[index].type = TYPE_DIR;
            dir_buf.entries[index].valid = 1;
            strcpy(dir_buf.entries[index].name, dirname);
            // update directory inode
            inode_dir.size += sizeof (struct dirent);
            // create child directory inode
            inode_chddir.size = 2 * sizeof (struct dirent);
            inode_chddir.type = TYPE_DIR;
            inode_chddir.link = 1;
            inode_chddir.ptr[0] = bmap_index;
            // initialize child directory block
            memset(&chddir_buf, 0, sizeof (struct dirblk));
            // "."
            chddir_buf.entries[0].index = imap_index;
            chddir_buf.entries[0].valid = 1;
            chddir_buf.entries[0].type = TYPE_DIR;
            memcpy(chddir_buf.entries[0].name, curdir, 2);
            // ".."
            chddir_buf.entries[1].index = index_dir;
            chddir_buf.entries[1].valid = 1;
            chddir_buf.entries[1].type = TYPE_DIR;
            memcpy(chddir_buf.entries[1].name, prtdir, 3);
            memcpy(fs_buf, &chddir_buf, FS_BLOCK_SIZE);
            if (fs_wr_block(DATA_BEGIN + bmap_index, fs_buf) == -1)
                return -1;
            // commit superblock changes
            memcpy(fs_buf, &spblock,  sizeof (struct superblock));
            if (fs_wr_block(0, fs_buf) == -1)
                return -1;
            // commit child directory inode changes
            if (wr_inode(imap_index, &inode_chddir) == -1)
                return -1;
            // commit directory entry changes
            memcpy(fs_buf, &dir_buf, FS_BLOCK_SIZE);
            if (fs_wr_block(DATA_BEGIN + inode_dir.ptr[i], fs_buf) == -1)
                return -1;
            // commit directory inode changes
            if (wr_inode(index_dir, &inode_dir) == -1)
                return -1;
            return imap_index;
        }
        ++i;
        size -= FS_BLOCK_SIZE;
    }
    // allocate a new data block for directory entry
    if (i < N_DIRECT_PTR)
    {
        // read-modify superblock
        if (fs_rd_block(0, fs_buf) == -1)
            return -1;
        memcpy(&spblock, fs_buf,  sizeof (struct superblock));
        if ((prtdir_bmap_index = bmap_lookup(&spblock)) == -1)
            return -1;
        bmap_set(prtdir_bmap_index, &spblock);
        if ((bmap_index = bmap_lookup(&spblock)) == -1)
            return -1;
        if ((imap_index = imap_lookup(&spblock)) == -1)
            return -1;
        bmap_set(bmap_index, &spblock);
        imap_set(imap_index, &spblock);
        spblock.free_block_count -= 2;
        spblock.free_inode_count -= 1;
        spblock.dir_inode_count += 1;
        // initialize new directory block
        memset(&dir_buf, 0, sizeof (struct dirblk));
        dir_buf.entries[0].index = imap_index;
        dir_buf.entries[0].type = TYPE_DIR;
        dir_buf.entries[0].valid = 1;
        strcpy(dir_buf.entries[0].name, dirname);
        // update directory inode
        inode_dir.size += sizeof (struct dirent);
        inode_dir.ptr[i] = prtdir_bmap_index;
        // create child directory inode
        inode_chddir.size = 2 * sizeof (struct dirent);
        inode_chddir.type = TYPE_DIR;
        inode_chddir.link = 1;
        inode_chddir.ptr[0] = bmap_index;

        // initialize child directory block
        memset(&chddir_buf, 0, sizeof (struct dirblk));
        // "."
        chddir_buf.entries[0].index = imap_index;
        chddir_buf.entries[0].valid = 1;
        chddir_buf.entries[0].type = TYPE_DIR;
        memcpy(chddir_buf.entries[0].name, curdir, 2);
        // ".."
        chddir_buf.entries[1].index = index_dir;
        chddir_buf.entries[1].valid = 1;
        chddir_buf.entries[1].type = TYPE_DIR;
        memcpy(chddir_buf.entries[1].name, prtdir, 3);
        memcpy(fs_buf, &chddir_buf, FS_BLOCK_SIZE);
        if (fs_wr_block(DATA_BEGIN + bmap_index, fs_buf) == -1)
            return -1;
        // commit new parent directory block
        memcpy(fs_buf, &dir_buf, FS_BLOCK_SIZE);
        if (fs_wr_block(DATA_BEGIN + prtdir_bmap_index, fs_buf) == -1)
            return -1;
        // commit superblock changes
        memcpy(fs_buf, &spblock,  sizeof (struct superblock));
        if (fs_wr_block(0, fs_buf) == -1)
            return -1;
        // commit child directory inode
        if (wr_inode(imap_index, &inode_chddir) == -1)
            return -1;
        // commit directory inode
        if (wr_inode(index_dir, &inode_dir) == -1)
            return -1;
        return imap_index;
    }
    return -1;
}

int readbyte(int index, int position)
{
    struct inode inode_buf;
    int blockno, offset;
    if (rd_inode(index, &inode_buf) < 0)
        return -1;
    if (position >= inode_buf.size || inode_buf.type == TYPE_DIR)
        return -1;
    blockno = position / FS_BLOCK_SIZE;
    offset = position % FS_BLOCK_SIZE;
    if (fs_rd_block(DATA_BEGIN + inode_buf.ptr[blockno], fs_buf) < 0)
        return -1;
    return fs_buf[offset];
}

int appendbyte(int index, char byte)
{
    struct inode inode_buf;
    int endblockno, endoffset; // position of the end of the file
    if (rd_inode(index, &inode_buf) < 0)
        return -1;
    if (inode_buf.size == FS_BLOCK_SIZE * N_DIRECT_PTR || inode_buf.type == TYPE_DIR)
        return -1;
    endblockno = (inode_buf.size - 1) / FS_BLOCK_SIZE;
    endoffset = (inode_buf.size - 1) % FS_BLOCK_SIZE;
    if (endoffset == FS_BLOCK_SIZE - 1)
    {
        // superblock modification
        struct superblock spblock;
        int bmap_index;
        if (fs_rd_block(0, fs_buf) < 0)
            return -1;
        memcpy(&spblock, fs_buf,  sizeof (struct superblock));
        if ((bmap_index = bmap_lookup(&spblock)) < 0)
            return -1;
        spblock.free_block_count--;
        bmap_set(bmap_index, &spblock);
        // inode modification
        inode_buf.size++;
        inode_buf.ptr[endblockno + 1] = bmap_index;
        // append byte
        memset(fs_buf, 0, FS_BLOCK_SIZE);
        fs_buf[0] = byte;
        if (fs_wr_block(DATA_BEGIN + bmap_index, fs_buf) < 0)
            return -1;
        // commit superblock changes
        memset(fs_buf, 0, FS_BLOCK_SIZE);
        memcpy(fs_buf, &spblock, sizeof (struct superblock));
        if (fs_wr_block(0, fs_buf) < 0)
            return -1;
        // commit inode changes
        if (wr_inode(index, &inode_buf) < 0)
            return -1;
    }
    else
    {
        // inode modification
        inode_buf.size++;
        // read-modify-write
        if (fs_rd_block(DATA_BEGIN + inode_buf.ptr[endblockno], fs_buf) < 0)
            return -1;
        fs_buf[endoffset + 1] = byte;
        if (fs_wr_block(DATA_BEGIN + inode_buf.ptr[endblockno], fs_buf) < 0)
            return -1;
        // commit inode change
        if (wr_inode(index, &inode_buf) < 0)
            return -1;
    }
    return 0;
}

int writebyte(int index, int position, char byte)
{
    struct inode inode_buf;
    int blockno, offset;
    int endblockno, endoffset; // position of the end of the file
    if (rd_inode(index, &inode_buf) < 0)
        return -1;
    if (position >= FS_BLOCK_SIZE * N_DIRECT_PTR || inode_buf.type == TYPE_DIR)
        return -1;
    blockno = position / FS_BLOCK_SIZE;
    offset = position % FS_BLOCK_SIZE;
    if (position < inode_buf.size)
    {
        // read-modify-write
        if (fs_rd_block(DATA_BEGIN + inode_buf.ptr[blockno], fs_buf) < 0)
            return -1;
        fs_buf[offset] = byte;
        if (fs_wr_block(DATA_BEGIN + inode_buf.ptr[blockno], fs_buf) < 0)
            return -1;
    }
    else
    {
        int i = inode_buf.size;
        // not efficient yet simple to implement
        // can be further optimized
        while (i++ < position)
            appendbyte(index, '\0');
        appendbyte(index, byte);
    }
}

int clone(int src_inodeno, int dst_inodeno)
{
    struct inode src_inode, dst_inode;
    struct superblock spblock;
    int bmap_index;
    if (rd_inode(src_inodeno, &src_inode) < 0)
        return -1;
    if (rd_inode(dst_inodeno, &dst_inode) < 0)
        return -1;
    int src_blockcnt = (src_inode.size-1) / FS_BLOCK_SIZE + 1;
    int dst_blockcnt = (dst_inode.size-1) / FS_BLOCK_SIZE + 1;
    int i;
    if (fs_rd_block(0, fs_buf) < 0)
        return -1;
    memcpy(&spblock, fs_buf,  sizeof (struct superblock));
    for (i=0; i<src_blockcnt; ++i)
    {
        if (i < dst_blockcnt)
        {
            if (fs_rd_block(DATA_BEGIN + src_inode.ptr[i], fs_buf) < 0)
                return -1;
            if (fs_wr_block(DATA_BEGIN + dst_inode.ptr[i], fs_buf) < 0)
                return -1;
        }
        else // need to allocate new data blocks
        {
            // superblock modification
            if ((bmap_index = bmap_lookup(&spblock)) < 0)
                return -1;
            spblock.free_block_count--;
            bmap_set(bmap_index, &spblock);
            // copy data block
            dst_inode.ptr[i] = bmap_index;
            if (fs_rd_block(DATA_BEGIN + src_inode.ptr[i], fs_buf) < 0)
                return -1;
            if (fs_wr_block(DATA_BEGIN + dst_inode.ptr[i], fs_buf) < 0)
                return -1;
        }
    }
    // commit superblock changes
    memset(fs_buf, 0, FS_BLOCK_SIZE);
    memcpy(fs_buf, &spblock, sizeof (struct superblock));
    if (fs_wr_block(0, fs_buf) < 0)
        return -1;
    dst_inode.size = src_inode.size;
    // commit inode changes
    if (wr_inode(dst_inodeno, &dst_inode) < 0)
        return -1;
    return 0;
}

int openpath(const char* path)
{
    static char filename[256];
    struct inode current_inode, next_inode;
    struct dirblk dirents;
    int i, j, size, index;
    if (path[0] != '/')
        return -1;
    const char* p = path;
    rd_inode(0, &current_inode);
    if (fs_rd_block(DATA_BEGIN, (char*) &dirents) < 0)
        return -1;
    index = 0;
    while (*p != '\0')
    {
        ++p;
        if (current_inode.type == TYPE_FILE)
            return -1;
        if (*p == '\0')
            return dirents.entries[index].index;
        const char* q = p;
        char* ptr_fn = filename;
        while (*q != '/' && *q != '\0')
            *ptr_fn++ = *q++;
        *ptr_fn = '\0';
        i = 0;
        size = current_inode.size;
        while (size > 0)
        {
            if (fs_rd_block(DATA_BEGIN + current_inode.ptr[i], (char*) &dirents) < 0)
                return -1;
            if ((index = dirent_lookup(&dirents, filename)) >= 0)
            {
                if (rd_inode(dirents.entries[index].index, &next_inode) < 0)
                    return -1;
                break;
            }
            size -= FS_BLOCK_SIZE;
            ++i;
        }
        if (size < 0)
            return -1;
        memcpy(&current_inode, &next_inode, sizeof (struct inode));
        p = q;
    }
    return dirents.entries[index].index;
}
