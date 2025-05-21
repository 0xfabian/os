#pragma once

#include <cstdint>
#include <cstddef>
#include <print.h>
#include <error.h>

#define IT_MASK     0xf000
#define IT_FIFO     0x1000
#define IT_CDEV     0x2000
#define IT_DIR      0x4000
#define IT_BDEV     0x6000
#define IT_REG      0x8000
#define IT_LINK     0xa000
#define IT_SOCK     0xc000

#define IP_MASK     0x01ff
#define IP_R_U      0x0100
#define IP_W_U      0x0080
#define IP_X_U      0x0040
#define IP_R_G      0x0020
#define IP_W_G      0x0010
#define IP_X_G      0x0008
#define IP_R_O      0x0004
#define IP_W_O      0x0002
#define IP_X_O      0x0001
#define IP_R        (IP_R_U | IP_R_G | IP_R_O)
#define IP_W        (IP_W_U | IP_W_G | IP_W_O)
#define IP_X        (IP_X_U | IP_X_G | IP_X_O)
#define IP_RW       (IP_R | IP_W)
#define IP_RWX      (IP_R | IP_W | IP_X)

#define IF_ALLOC        1
#define IF_PERSISTENT   2

struct Superblock;
struct Inode;
struct File;

struct InodeOps
{
    int (*create) (Inode* dir, const char* name, u32 mode);
    int (*mknod) (Inode* dir, const char* name, u32 mode, u32 dev);
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
    isize(*read) (File* file, void* buf, usize size, usize offset);
    isize(*write) (File* file, const void* buf, usize size, usize offset);
    usize(*seek) (File* file, usize offset, int whence);
    int (*iterate) (File* file, void* buf, usize size);
    int (*ioctl) (File* file, int cmd, void* arg);
};

struct stat
{
    u64 st_dev;
    u64 st_ino;
    u64 st_nlink;
    u32 st_mode;
    u32 st_uid;
    u32 st_gid;
    int __pad0;
    u64 st_rdev;
    i64 st_size;
    i64 st_blksize;
    i64 st_blocks;
    i64 st_atime;
    u64 st_atimensec;
    i64 st_mtime;
    u64 st_mtimensec;
    i64 st_ctime;
    u64 st_ctimensec;
    i64 __reserved[3];
};

struct Inode
{
    Superblock* sb;
    u64 ino;
    u32 mode;
    u32 dev;
    usize size;
    void* data;
    int nlinks;
    int refs;
    u8 flags;

    InodeOps ops;
    FileOps fops;

    static result_ptr<Inode> get(const char* path);
    Inode* get();
    void put();

    bool is_reg();
    bool is_dir();
    bool is_device();
    bool is_block_device();
    bool is_char_device();
    bool is_readable();
    bool is_writable();
    bool is_executable();

    int create(const char* name, u32 mode);
    int mknod(const char* name, u32 mode, u32 dev);
    int link(const char* name, Inode* inode);
    int unlink(const char* name);
    int mkdir(const char* name);
    int rmdir(const char* name);
    int truncate(usize size);
    result_ptr<Inode> lookup(const char* name);
    int sync();

    int fill_stat(stat* buf);
};

struct Dirent
{
    u64 ino;
    u32 type;
    char name[32];
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