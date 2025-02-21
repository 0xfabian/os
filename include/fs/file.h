#pragma once

#include <print.h>
#include <fs/inode.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct File
{
    size_t offset;
    Inode* inode;
    uint32_t flags;
    int refs;

    FileOps ops;

    static result_ptr<File> open(const char* path, uint32_t flags);
    int close();

    int read(char* buf, size_t size);
    int write(const char* buf, size_t size);
    size_t seek(size_t offset, int whence);
    int iterate(void* buf, size_t size);
};

#define FILE_TABLE_SIZE 256

struct FileTable
{
    File files[FILE_TABLE_SIZE];

    result_ptr<File> alloc();

    void debug();
};

extern FileTable file_table;

size_t generic_seek(File* file, size_t offset, int whence);