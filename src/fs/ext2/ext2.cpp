#include <fs/ext2/ext2.h>

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
    sb->dev->read(gd->inode_table * ext2_sb->block_size + index * sizeof(Ext2Inode), (u8*)inode, sizeof(Ext2Inode));

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

    Inode root;
    root.sb = sb;
    root.type = IT_DIR;
    root.ops = ext2_dir_inode_ops;
    root.fops = ext2_dir_file_ops;
    root.data = kmalloc(sizeof(Ext2Inode));
    ext2_read_inode(sb, 2, (Ext2Inode*)root.data);
    root.nlinks = ((Ext2Inode*)root.data)->nlinks;
    root.size = ((Ext2Inode*)root.data)->size_low;
    root.flags = 0;

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
            result->type = ext2_type_mapping[dirent->type];
            result->data = kmalloc(sizeof(Ext2Inode));
            ext2_read_inode(result->sb, dirent->ino, (Ext2Inode*)result->data);
            result->nlinks = ((Ext2Inode*)result->data)->nlinks;
            result->size = ((Ext2Inode*)result->data)->size_low;
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

u32 logical_block_to_physical(Ext2Inode* inode, BlockDevice* bdev, u32 bn)
{
    if (bn < 12)
        return inode->block_ptr[bn];

    bn -= 12;

    if (bn < 1024)
    {
        u32 block_ptr[1024];
        bdev->read(inode->block_ptr[12] * 4096, (u8*)block_ptr, 4096);

        return block_ptr[bn];
    }

    return 0;
}

isize ext2_read(File* file, void* buf, usize size, usize offset)
{
    Ext2Inode* inode = (Ext2Inode*)file->inode->data;

    if (size == 0 || offset >= inode->size_low)
        return 0;

    usize remaining = inode->size_low - offset;

    if (size > remaining)
        size = remaining;

    u32 bn = offset / 4096;
    u32 block = logical_block_to_physical(inode, file->inode->sb->dev, bn);

    if (!block)
        return -1;

    u32 off = offset % 4096;
    u32 read = 0;

    while (read < size)
    {
        u32 to_read = size - read;

        if (4096 - off < to_read)
            to_read = 4096 - off;

        file->inode->sb->dev->read(block * 4096 + off, (u8*)buf + read, to_read);

        read += to_read;
        bn++;
        block = logical_block_to_physical(inode, file->inode->sb->dev, bn);

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

};

InodeOps ext2_dir_inode_ops =
{
    .lookup = ext2_lookup,
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