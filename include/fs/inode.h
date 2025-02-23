#pragma once

#include <cstdint>
#include <cstddef>
#include <print.h>
#include <error.h>

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
    int (*create) (Inode* dir, const char* name);
    int (*mknod) (Inode* dir, const char* name, uint32_t dev);
    int (*link) (Inode* dir, const char* name, Inode* inode);
    int (*unlink) (Inode* dir, const char* name);
    int (*mkdir) (Inode* dir, const char* name);
    int (*rmdir) (Inode* dir, const char* name);
    int (*truncate) (Inode* inode, size_t size);
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
    int (*ioctl) (File* file, int cmd, void* arg);
};

struct Inode
{
    Superblock* sb;
    uint64_t ino;
    uint32_t type;
    uint32_t dev;
    size_t size;
    void* data;
    int nlinks;
    int refs;

    InodeOps ops;
    FileOps fops;

    static result_ptr<Inode> get(const char* path);
    void put();

    bool is_reg();
    bool is_dir();
    bool is_device();
    bool is_block_device();
    bool is_char_device();

    int create(const char* name);
    int mknod(const char* name, uint32_t dev);
    int link(const char* name, Inode* inode);
    int unlink(const char* name);
    int mkdir(const char* name);
    int rmdir(const char* name);
    int truncate(size_t size);
    result_ptr<Inode> lookup(const char* name);
    int sync();
};

#define INODE_TABLE_SIZE 256

struct InodeTable
{
    Inode inodes[INODE_TABLE_SIZE];

    result_ptr<Inode> insert(Inode* inode);
    result_ptr<Inode> find(Superblock* sb, uint64_t ino);
    size_t get_sb_refs(Superblock* sb);

    void debug();
};

extern InodeTable inode_table;