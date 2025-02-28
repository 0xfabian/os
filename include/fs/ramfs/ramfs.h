#pragma once

#include <mem/heap.h>
#include <fs/fstype.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/file.h>

extern Filesystem ramfs;

extern InodeOps ramfs_reg_inode_ops;
extern InodeOps ramfs_dir_inode_ops;

extern FileOps ramfs_reg_file_ops;
extern FileOps ramfs_dir_file_ops;

result_ptr<Superblock> ramfs_create_sb(Filesystem* fs, Device* dev);
void ramfs_destroy_sb(Superblock* sb);

int ramfs_create(Inode* dir, const char* name);
int ramfs_mknod(Inode* dir, const char* name, u32 dev);
int ramfs_link(Inode* dir, const char* name, Inode* inode);
int ramfs_unlink(Inode* dir, const char* name);
int ramfs_mkdir(Inode* dir, const char* name);
int ramfs_rmdir(Inode* dir, const char* name);
int ramfs_truncate(Inode* inode, usize size);
int ramfs_lookup(Inode* _dir, const char* name, Inode* result);
int ramfs_sync(Inode* inode);

int ramfs_read(File* file, char* buf, usize size, usize offset);
int ramfs_write(File* file, const char* buf, usize size, usize offset);
int ramfs_iterate(File* file, void* buf, usize size);