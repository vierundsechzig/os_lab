#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"
#include "commands.h"

// 返回文件名的第一个字符的位置，参考自xv6的ls.c源代码
const char* filename(const char* path)
{
    const char* p;
    for (p = path + strlen(path); p >= path && *p != '/'; --p)
        ;
    ++p;
    return p;
}

void format_c()
{
    char ch;
    printf("All data will be LOST. Are you ABSOLUTELY sure? (1 to continue)");
    ch = getchar();
    getchar();
    if (ch != '1')
        return;
    if (format() == 0)
        puts("format: completed");
    else
        puts("format: errors occurred");
}

void ls_c(const char* path)
{
    struct inode inode_root_dir;
    struct dirblk dirents;
    int inodeno;
    if ((inodeno = openpath(path)) < 0)
    {
        printf("ls: open %s failed\n", path);
        return;
    }
    if (rd_inode(inodeno, &inode_root_dir) < 0)
    {
        puts("ls: load root directory inode failed");
        return;
    }
    int size = inode_root_dir.size;
    int i = 0;
    int j;
    while (size > 0)
    {
        if (fs_rd_block(DATA_BEGIN + inode_root_dir.ptr[i], (char*) &dirents) < 0)
        {
            puts("ls: load root directory data block failed");
            return;
        }
        for (j=0; j<FS_BLOCK_SIZE / sizeof (struct dirent); ++j)
        {
            if (dirents.entries[j].valid)
            {
                printf("%s %d", dirents.entries[j].name, dirents.entries[j].index);
                if (dirents.entries[j].type == TYPE_DIR)
                    printf(" <DIR>");
                putchar('\n');
            }
        }
        size -= FS_BLOCK_SIZE;
        ++i;
    }
}

void mkdir_c(const char* path)
{
    const char* newdir = filename(path);
    if (*newdir == '\0')
    {
        puts("mkdir: directory name should not be empty");
        return;
    }
    int position = newdir - path;
    char prtdir[256];
    memcpy(prtdir, path, position);
    prtdir[position] = '\0';
    int inode = openpath(prtdir);
    if (inode < 0)
    {
        puts("mkdir: open directory failed");
        return;
    }
    if (mkdir(inode, newdir) < 0)
    {
        puts("mkdir: make new directory failed");
        return;
    }
}

void touch_c(const char* path)
{
    const char* newfile = filename(path);
    if (*newfile == '\0')
    {
        puts("mkdir: file name should not be empty");
        return;
    }
    int position = newfile - path;
    char prtdir[256];
    memcpy(prtdir, path, position);
    prtdir[position] = '\0';
    int inode = openpath(prtdir);
    if (inode < 0)
    {
        puts("touch: open destination directory failed");
        return;
    }
    if (touch(inode, newfile) < 0)
    {
        puts("touch: create new file failed");
        return;
    }
}

void cp_c(const char* dst, const char* src)
{
    // source
    int src_inodeno;
    if ((src_inodeno = openpath(src)) < 0)
    {
        printf("cp: open %s failed\n", src);
        return;
    }
    // destination
    const char* dst_file = filename(dst);
    if (*dst_file == '\0')
    {
        puts("cp: file name should not be empty");
        return;
    }
    int position = dst_file - dst;
    char prtdir[256];
    memcpy(prtdir, dst, position);
    prtdir[position] = '\0';
    int dstdir_inodeno = openpath(prtdir);
    if (dstdir_inodeno < 0)
    {
        printf("cp: open destination directory failed");
        return;
    }
    // copy
    int dst_inodeno;
    if ((dst_inodeno = touch(dstdir_inodeno, dst_file)) < 0)
    {
        puts("cp: create new file failed");
        return;
    }
    if (clone(src_inodeno, dst_inodeno) < 0)
    {
        puts("cp: copy file failed");
        return;
    }
}

void exit_c()
{
    exit(0);
}

void tee_c(const char* path)
{
    int inodeno;
    struct inode file_inode;
    if ((inodeno = openpath(path)) < 0)
    {
        printf("tee: open %s failed\n", path);
        return;
    }
    if (rd_inode(inodeno, &file_inode) < 0)
    {
        puts("tee: read inode failed");
        return;
    }
    if (file_inode.type == TYPE_DIR)
    {
        puts("tee: cannot write a directory");
        return;
    }
    char prev_ch;
    char ch;
    int i = 0;
    while ((ch = getchar()) >= 0)
    {
        if (i >= FS_BLOCK_SIZE * N_DIRECT_PTR) // 大于最大文件长度
            break;
        if (ch == '\n' && prev_ch == '\n')
            break;
        writebyte(inodeno, i, ch);
        ++i;
        prev_ch = ch;
    }
}

void cat_c(const char* path)
{
    int inodeno;
    struct inode file_inode;
    if ((inodeno = openpath(path)) < 0)
    {
        printf("cat: open %s failed\n", path);
        return;
    }
    if (rd_inode(inodeno, &file_inode) < 0)
    {
        puts("cat: read inode failed");
        return;
    }
    if (file_inode.type == TYPE_DIR)
    {
        puts("cat: cannot write a directory");
        return;
    }
    int i;
    for (i=0; i<file_inode.size; ++i)
        putchar(readbyte(inodeno, i));
}

void help_c()
{
    puts("ls: list all contents of a directory");
    puts("mkdir: create a blank directory");
    puts("touch: create a blank file");
    puts("cp: copy a file");
    puts("tee: write a file");
    puts("cat: read a file");
    puts("help: show this help");
    puts("stat: show information of a file or directory");
    puts("format: deploy a fresh new file system");
    puts("exit: exit this program");
}

void stat_c(const char* path)
{
    int inodeno;
    struct inode file_inode;
    const char* inode_type[2] = {"FILE", "DIR"};
    if ((inodeno = openpath(path)) < 0)
    {
        printf("stat: open %s failed\n", path);
        return;
    }
    if (rd_inode(inodeno, &file_inode) < 0)
    {
        puts("stat: read inode failed");
        return;
    }
    printf("Type: %s\n", inode_type[file_inode.type == TYPE_DIR]);
    printf("Size: %d\n", file_inode.size);
    printf("Links: %d\n", file_inode.link);
    for (int i=0; i<N_DIRECT_PTR; ++i)
        printf("Pointer %d: %d\n", i, file_inode.ptr[i]);
}

void exec(const char* cmd)
{
    int argc = 0;
    static char argv[3][256];
    const char* ptr_cmd;
    char* ptr_argv;
    ptr_cmd = cmd;
    memset(argv[0], 0, 256);
    memset(argv[1], 0, 256);
    memset(argv[2], 0, 256);
    while (*ptr_cmd == ' ')
        ++ptr_cmd;
    // 最多接受三个参数
    while (argc < 3)
    {
        ptr_argv = argv[argc];
        while (*ptr_cmd != ' ' && *ptr_cmd != '\0')
        {
            *ptr_argv = *ptr_cmd;
            ++ptr_argv;
            ++ptr_cmd;
        }
        *ptr_argv = '\0';
        while (*ptr_cmd == ' ')
            ++ptr_cmd;
        ++argc;
        if (*ptr_cmd == '\0')
            break;
    }
    if (strcmp(argv[0], "ls") == 0)
    {
        if (argc == 1)
            ls_c("/");
        else if (argc == 2)
            ls_c(argv[1]);
        else
            puts("ls: too many arguments");
    }
    else if (strcmp(argv[0], "mkdir") == 0)
    {
        if (argc == 1)
            puts("mkdir: missing the path");
        else if (argc == 2)
            mkdir_c(argv[1]);
        else
            puts("mkdir: too many arguments");
    }
    else if (strcmp(argv[0], "touch") == 0)
    {
        if (argc == 1)
            puts("touch: missing the path");
        else if (argc == 2)
            touch_c(argv[1]);
        else
            puts("touch: too many arguments");
    }
    else if (strcmp(argv[0], "cp") == 0)
    {
        if (argc <= 2)
            puts("cp: too few arguments");
        else
            cp_c(argv[2], argv[1]);
    }
    else if (strcmp(argv[0], "exit") == 0)
        exit_c();
    else if (strcmp(argv[0], "help") == 0)
        help_c();
    else if (strcmp(argv[0], "tee") == 0)
    {
        if (argc == 1)
            puts("tee: missing the path");
        else if (argc == 2)
            tee_c(argv[1]);
        else
            puts("tee: too many arguments");
    }
    else if (strcmp(argv[0], "cat") == 0)
    {
        if (argc == 1)
            puts("cat: missing the path");
        else if (argc == 2)
            cat_c(argv[1]);
        else
            puts("cat: too many arguments");
    }
    else if (strcmp(argv[0], "stat") == 0)
    {
        if (argc == 1)
            puts("stat: missing the path");
        else if (argc == 2)
            stat_c(argv[1]);
        else
            puts("stat: too many arguments");
    }
    else if (strcmp(argv[0], "format") == 0)
        format_c();
    else
        printf("exec %s failed\n", argv[0]);
}
