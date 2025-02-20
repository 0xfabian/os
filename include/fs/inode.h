#pragma once

#include <cstdint>
#include <cstddef>
#include <print.h>

#define IT_FIFO 0x1000
#define IT_CDEV 0x2000
#define IT_DIR  0x4000
#define IT_BDEV 0x6000
#define IT_REG  0x8000
#define IT_LINK 0xA000
#define IT_SOCK 0xC000

struct Superblock;
struct Inode;
struct File;

struct InodeOps
{
    // create
    // mknod
    // rename
    // link
    // unlink
    // rmdir
    // update_time
    // truncate

    int (*mkdir) (Inode* dir, const char* name);
    int (*lookup) (Inode* dir, const char* name, Inode* result);
    int (*sync) (Inode* inode);
};

struct FileOps
{
    int (*open) (File* file);
    int (*close) (File* file);
    int (*read) (File* file, char* buf, size_t size, size_t offset);
    int (*write) (File* file, const char* buf, size_t size, size_t offset);
    size_t(*seek) (File* file, size_t offset, int whence);
    int (*iterate) (File* file, void* buf, size_t size);
    // ioctl
    // mmap
};

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

    int mkdir(const char* name);
    Inode* lookup(const char* name);
    int sync();
};

#define INODE_TABLE_SIZE 256

struct InodeTable
{
    Inode inodes[INODE_TABLE_SIZE];

    Inode* insert(Inode* inode);
    size_t get_sb_refs(Superblock* sb);

    void debug();
};

extern InodeTable inode_table;