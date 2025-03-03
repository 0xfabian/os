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

#define IF_ALLOC        1
#define IF_PERSISTENT   2

struct Superblock;
struct Inode;
struct File;

struct InodeOps
{
    int (*create) (Inode* dir, const char* name);
    int (*mknod) (Inode* dir, const char* name, u32 dev);
    int (*link) (Inode* dir, const char* name, Inode* inode);
    int (*unlink) (Inode* dir, const char* name);
    int (*mkdir) (Inode* dir, const char* name);
    int (*rmdir) (Inode* dir, const char* name);
    int (*truncate) (Inode* inode, usize size);
    int (*lookup) (Inode* dir, const char* name, Inode* result);
    int (*sync) (Inode* inode);
};

struct FileOps
{
    int (*open) (File* file);
    int (*close) (File* file);
    isize(*read) (File* file, char* buf, usize size, usize offset);
    isize(*write) (File* file, const char* buf, usize size, usize offset);
    usize(*seek) (File* file, usize offset, int whence);
    int (*iterate) (File* file, void* buf, usize size);
    int (*ioctl) (File* file, int cmd, void* arg);
};

struct Inode
{
    Superblock* sb;
    u64 ino;
    u32 type;
    u32 dev;
    usize size;
    void* data;
    int nlinks;
    int refs;
    u8 flags;

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
    int mknod(const char* name, u32 dev);
    int link(const char* name, Inode* inode);
    int unlink(const char* name);
    int mkdir(const char* name);
    int rmdir(const char* name);
    int truncate(usize size);
    result_ptr<Inode> lookup(const char* name);
    int sync();
};

#define INODE_TABLE_SIZE 256

struct InodeTable
{
    Inode inodes[INODE_TABLE_SIZE];

    result_ptr<Inode> insert(Inode* inode);
    result_ptr<Inode> find(Superblock* sb, u64 ino);
    usize get_sb_refs(Superblock* sb);

    void debug();
};

extern InodeTable inode_table;