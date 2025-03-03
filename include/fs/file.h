#pragma once

#include <print.h>
#include <fs/inode.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct File
{
    usize offset;
    Inode* inode;
    u32 flags;
    int refs;

    FileOps ops;

    static result_ptr<File> open(const char* path, u32 flags);
    int close();

    isize read(char* buf, usize size);
    isize write(const char* buf, usize size);
    usize seek(usize offset, int whence);
    int iterate(void* buf, usize size);
    int ioctl(int cmd, void* arg);
};

#define FILE_TABLE_SIZE 256

struct FileTable
{
    File files[FILE_TABLE_SIZE];

    result_ptr<File> alloc();

    void debug();
};

extern FileTable file_table;

usize generic_seek(File* file, usize offset, int whence);