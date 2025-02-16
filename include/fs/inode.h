// #pragma once

// #include <superblock.h>

// struct InodeOps
// {
//     int (*create) (Inode* inode, const char* name, uint32_t mode);
//     int (*lookup) (Inode* inode, const char* name, Inode* result);
//     int (*link) (Inode* inode, const char* name);
//     int (*unlink) (Inode* inode);
//     int (*mkdir) (Inode* inode, const char* name);
//     int (*rmdir) (Inode* inode, const char* name);
//     int (*mknod) (Inode* inode, const char* name, uint32_t mode, uint64_t dev);
// };

// struct Inode
// {
//     Superblock* sb;
//     uint64_t ino;
//     uint32_t mode;
//     size_t size;
//     int refs;

//     InodeOps ops;

//     bool is_device();
// };