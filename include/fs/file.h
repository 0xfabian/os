#pragma once

#include <print.h>
#include <fs/inode.h>

struct File
{
    size_t offset;
    Inode* inode;
    uint32_t flags;
    int refs;

    FileOps ops;

    static File* open(const char* path, uint32_t flags);
    int close();

    int read(char* buf, size_t size);
    int write(const char* buf, size_t size);
    int seek(size_t offset, int whence);
};

#define FILE_TABLE_SIZE 256

struct FileTable
{
    File files[FILE_TABLE_SIZE];

    File* alloc();

    void debug();
};

extern FileTable file_table;