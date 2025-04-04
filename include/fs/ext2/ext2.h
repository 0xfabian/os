#pragma once

#include <mem/heap.h>
#include <fs/fstype.h>
#include <fs/superblock.h>
#include <fs/inode.h>
#include <fs/file.h>

struct Ext2DiskSuperblock
{
    u32 total_inodes;
    u32 total_blocks;
    u32 reserved_blocks;
    u32 free_blocks;
    u32 free_inodes;
    u32 sb_block_nr;
    u32 block_size;
    u32 fragment_size;
    u32 blocks_per_group;
    u32 fragments_per_group;
    u32 inodes_per_group;
    u32 last_mount_time;
    u32 last_write_time;
    u16 mounts_since_check;
    u16 max_mounts_before_check;
    u16 magic;
    u16 state;
    u16 errors;
    u16 minor_version;
    u32 last_check_time;
    u32 check_interval;
    u32 os_id;
    u32 major_version;
    u16 uid;
    u16 gid;

    u32 first_inode;
    u16 inode_size;
    u16 sb_block_group;
    u32 feature_compat;
    u32 feature_incompat;
    u32 feature_ro_compat;
    u8 uuid[16];
    char volume_name[16];
    char last_mount_path[64];
    u32 compression;
    u8 prealloc_blocks;
    u8 prealloc_dir_blocks;
    u16 padding;
    u8 journal_uuid[16];
    u32 journal_ino;
    u32 journal_dev;
    u32 orphan_head;
    u8 unused[788];
}
__attribute__((packed));

struct Ext2GroupDesc
{
    u32 block_bitmap;
    u32 inode_bitmap;
    u32 inode_table;
    u16 free_blocks;
    u16 free_inodes;
    u16 dirs;
    u8 unused[14];
}
__attribute__((packed));

struct Ext2Superblock
{
    Ext2DiskSuperblock disk_sb;
    u32 block_size;
    u32 group_desc_count;
    Ext2GroupDesc* group_desc;
};

struct Ext2Inode
{
    u16 type;
    u16 uid;
    u32 size_low;
    u32 last_access_time;
    u32 creation_time;
    u32 last_mod_time;
    u32 deletion_time;
    u16 gid;
    u16 nlinks;
    u32 blocks;
    u32 flags;
    u32 os_spec1;
    u32 block_ptr[15];
    u32 generation;
    u32 file_acl;
    u32 size_high;
    u32 fragment_addr;
    u8 os_spec2[12];
}
__attribute__((packed));

struct Ext2Dirent
{
    u32 ino;
    u16 size;
    u8 name_len;
    u8 type;
    char name[0];
}
__attribute__((packed));

extern Filesystem ext2fs;

extern InodeOps ext2_reg_inode_ops;
extern InodeOps ext2_dir_inode_ops;

extern FileOps ext2_reg_file_ops;
extern FileOps ext2_dir_file_ops;

result_ptr<Superblock> ext2_create_sb(Filesystem* fs, BlockDevice* dev);
void ext2_destroy_sb(Superblock* sb);

int ext2_lookup(Inode* _dir, const char* name, Inode* result);
int ext2_sync(Inode* inode);

isize ext2_read(File* file, void* buf, usize size, usize offset);
int ext2_iterate(File* file, void* buf, usize size);