#pragma once

#include <cstdint>
#include <cstddef>
#include <print.h>

struct Superblock;
struct Inode;
struct File;

struct InodeOps
{
    int (*lookup) (Inode* inode, const char* name, Inode* result);
    int (*sync) (Inode* inode);
};

struct FileOps
{
    int (*open) (File* file);
    int (*close) (File* file);
    int (*read) (File* file, char* buf, size_t size, size_t offset);
    int (*write) (File* file, const char* buf, size_t size, size_t offset);
    int (*seek) (File* file, size_t offset, int whence);
};

#define IT_FIFO 0x1000
#define IT_CDEV 0x2000
#define IT_DIR  0x4000
#define IT_BDEV 0x6000
#define IT_REG  0x8000
#define IT_LINK 0xA000
#define IT_SOCK 0xC000

struct Inode
{
    Superblock* sb;
    uint64_t ino;
    uint32_t type;
    size_t size;
    int refs;

    InodeOps ops;
    FileOps fops;

    static Inode* get(const char* path);
    void put();

    bool is_reg();
    bool is_dir();
    bool is_device();
    bool is_block_device();
    bool is_char_device();

    Inode* lookup(const char* name);
};

#define INODE_TABLE_SIZE 256

struct InodeTable
{
    Inode inodes[INODE_TABLE_SIZE];

    Inode* insert(Inode* inode);

    void debug();
};

extern InodeTable inode_table;

// this should not be here
char* path_read_next(const char*& ptr);