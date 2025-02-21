#pragma once

#include <mem/heap.h>
#include <fs/fstype.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/file.h>

extern Filesystem ramfs;

result_ptr<Superblock> ramfs_create_sb(Filesystem* fs, Device* dev);
void ramfs_destroy_sb(Superblock* sb);

int ramfs_mkdir(Inode* dir, const char* name);
int ramfs_lookup(Inode* _dir, const char* name, Inode* result);

int ramfs_read(File* file, char* buf, size_t size, size_t offset);
int ramfs_iterate(File* file, void* buf, size_t size);