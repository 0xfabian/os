#include <fs/ext2/ext2.h>

// This assumes 4k blocks and 128 bytes inodes
// which is not always the case

u32 ext2_type_mapping[] =
{
    0,
    IT_REG,
    IT_DIR,
    IT_CDEV,
    IT_BDEV,
    IT_FIFO,
    IT_SOCK,
    IT_LINK
};

int ext2_read_inode(Superblock* sb, u32 ino, Ext2Inode* inode)
{
    Ext2Superblock* ext2_sb = (Ext2Superblock*)sb->data;

    u32 group = (ino - 1) / ext2_sb->disk_sb.inodes_per_group;
    u32 index = (ino - 1) % ext2_sb->disk_sb.inodes_per_group;

    Ext2GroupDesc* gd = &ext2_sb->group_desc[group];
    sb->dev->read(gd->inode_table * ext2_sb->block_size + index * ext2_sb->disk_sb.inode_size, (u8*)inode, sizeof(Ext2Inode));

    return 0;
}

result_ptr<Superblock> ext2_create_sb(Filesystem* fs, BlockDevice* dev)
{
    Superblock* sb = (Superblock*)kmalloc(sizeof(Superblock));

    sb->fs = fs;
    sb->dev = dev;
    sb->ops = { nullptr };

    Ext2Superblock* ext2_sb = (Ext2Superblock*)kmalloc(sizeof(Ext2Superblock));

    if (!dev->read(1024, (u8*)ext2_sb, sizeof(Ext2DiskSuperblock)))
    {
        kfree(sb);
        kfree(ext2_sb);

        return -1;
    }

    ext2_sb->block_size = 1024 << ext2_sb->disk_sb.block_size;
    ext2_sb->group_desc_count = (ext2_sb->disk_sb.total_blocks + ext2_sb->disk_sb.blocks_per_group - 1) / ext2_sb->disk_sb.blocks_per_group;
    ext2_sb->group_desc = (Ext2GroupDesc*)kmalloc(ext2_sb->group_desc_count * sizeof(Ext2GroupDesc));

    dev->read((ext2_sb->disk_sb.sb_block_nr + 1) * ext2_sb->block_size, (u8*)ext2_sb->group_desc, ext2_sb->group_desc_count * sizeof(Ext2GroupDesc));

    sb->data = ext2_sb;

    Ext2Inode* ext2_root = (Ext2Inode*)kmalloc(sizeof(Ext2Inode));
    ext2_read_inode(sb, 2, ext2_root);

    Inode root;
    root.sb = sb;
    root.ino = 2;
    root.mode = ext2_root->mode;
    root.dev = 0;
    root.size = ext2_root->size_low; // maybe or with size_high
    root.data = ext2_root;
    root.nlinks = ext2_root->nlinks;
    root.flags = 0;
    root.ops = ext2_dir_inode_ops;
    root.fops = ext2_dir_file_ops;

    sb->root = inode_table.insert(&root).ptr;

    return sb;
}

void ext2_destroy_sb(Superblock* sb)
{
    Ext2Superblock* ext2_sb = (Ext2Superblock*)sb->data;

    kfree(ext2_sb->group_desc);
    kfree(ext2_sb);
    kfree(sb);
}

int ext2_lookup(Inode* _dir, const char* name, Inode* result)
{
    Ext2Inode* dir = (Ext2Inode*)_dir->data;

    u8 buf[4096];
    _dir->sb->dev->read(dir->block_ptr[0] * 4096, buf, 4096);

    Ext2Dirent* dirent = (Ext2Dirent*)buf;
    u32 off = 0;

    while (off < 4096)
    {
        if (strncmp(name, dirent->name, dirent->name_len) == 0 && name[dirent->name_len] == 0)
        {
            result->sb = _dir->sb;
            result->ino = dirent->ino;

            // this avoid the memory leak caused by allocating a new Ext2Inode
            // and not freeing if the inode is already in the table
            // but is bad since this gets checked in inode_table.insert anyway
            if (inode_table.find(result->sb, result->ino))
                return 0;

            Ext2Inode* inode = (Ext2Inode*)kmalloc(sizeof(Ext2Inode));
            ext2_read_inode(result->sb, result->ino, inode);

            result->mode = inode->mode;
            result->dev = 0;
            result->size = inode->size_low; // maybe or with size_high
            result->data = inode;
            result->nlinks = inode->nlinks;
            result->flags = 0;

            if (result->is_dir())
            {
                result->ops = ext2_dir_inode_ops;
                result->fops = ext2_dir_file_ops;
            }
            else
            {
                result->ops = ext2_reg_inode_ops;
                result->fops = ext2_reg_file_ops;
            }

            return 0;
        }

        off += dirent->size;
        dirent = (Ext2Dirent*)(buf + off);
    }

    return -1;
}

int ext2_sync(Inode* inode)
{
    kfree(inode->data);

    return 0;
}

struct BlockCache
{
    u32 block;

    union
    {
        u8 data[4096];
        u32 ptrs[1024];
    };
};

BlockCache block_cache;
u32 logical_block_to_physical(Ext2Inode* inode, BlockDevice* bdev, u32 bn)
{
    if (bn < 12)
        return inode->block_ptr[bn];

    bn -= 12;

    if (bn < 1024)
    {
        if (block_cache.block != inode->block_ptr[12])
        {
            block_cache.block = inode->block_ptr[12];
            bdev->read(block_cache.block * 4096, block_cache.data, 4096);
        }

        return block_cache.ptrs[bn];
    }

    return 0;
}

isize ext2_read(File* file, void* buf, usize size, usize offset)
{
    Ext2Inode* inode = (Ext2Inode*)file->inode->data;

    if (size == 0 || offset >= inode->size_low)
        return 0;

    BlockDevice* bdev = file->inode->sb->dev;
    usize remaining = inode->size_low - offset;

    if (size > remaining)
        size = remaining;

    u32 bn = offset / 4096;
    u32 block = logical_block_to_physical(inode, bdev, bn);

    if (!block)
        return -1;

    u32 off = offset % 4096;
    u32 read = 0;

    while (read < size)
    {
        u32 to_read = size - read;

        if (4096 - off < to_read)
            to_read = 4096 - off;

        bdev->read(block * 4096 + off, (u8*)buf + read, to_read);

        read += to_read;
        bn++;
        block = logical_block_to_physical(inode, bdev, bn);

        if (!block)
            return read;

        off = 0;
    }

    return read;
}

int ext2_iterate(File* file, void* buf, usize size)
{
    if (file->offset == 4096)
        return 0;

    Ext2Inode* dir = (Ext2Inode*)file->inode->data;

    u8 temp[512];
    file->inode->sb->dev->read(dir->block_ptr[0] * 4096 + file->offset % 4096, temp, 512);

    Ext2Dirent* dirent = (Ext2Dirent*)temp;

    Dirent* d = (Dirent*)buf;
    d->ino = dirent->ino;
    d->type = ext2_type_mapping[dirent->type];
    memcpy(d->name, dirent->name, dirent->name_len);
    d->name[dirent->name_len] = 0;

    file->offset += dirent->size;

    return sizeof(Dirent);
}

Filesystem ext2fs =
{
    .name = "ext2",
    .flags = FS_REQUIRES_DEV,
    .create_sb = ext2_create_sb,
    .destroy_sb = ext2_destroy_sb
};

InodeOps ext2_reg_inode_ops =
{
    .sync = ext2_sync
};

InodeOps ext2_dir_inode_ops =
{
    .lookup = ext2_lookup,
    .sync = ext2_sync
};

FileOps ext2_reg_file_ops =
{
    .read = ext2_read,
    .seek = generic_seek
};

FileOps ext2_dir_file_ops =
{
    .seek = generic_seek,
    .iterate = ext2_iterate
};
