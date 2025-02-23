#include <fs/ramfs/ramfs.h>

struct RamDirent
{
    uint64_t ino;
    uint32_t type;
    char name[32];
};

int ramfs_writei(Inode* inode, const void* buf, size_t size, size_t offset)
{
    if (size == 0)
        return 0;

    if (offset + size > inode->size)
    {
        size_t newsize = offset + size;
        void* newdata = kmalloc(newsize);

        if (inode->data)
        {
            memcpy(newdata, inode->data, inode->size);
            kfree(inode->data);
        }

        if (offset > inode->size)
            memset((char*)newdata + inode->size, 0, offset - inode->size);

        inode->data = newdata;
        inode->size = newsize;
    }

    memcpy((char*)inode->data + offset, buf, size);

    return 0;
}

int ramfs_readi(Inode* inode, void* buf, size_t size, size_t offset)
{
    if (size == 0 || offset >= inode->size)
        return 0;

    size_t remaining = inode->size - offset;

    if (size > remaining)
        size = remaining;

    memcpy(buf, (char*)inode->data + offset, size);

    return size;
}

Inode* ramfs_alloc_inode()
{
    for (int i = 0; i < INODE_TABLE_SIZE; i++)
    {
        Inode* inode = &inode_table.inodes[i];

        if (inode->refs == 0 && inode->nlinks == 0)
        {
            inode->ino = i;
            return inode;
        }
    }

    return nullptr;
}

void ramfs_init(Superblock* sb)
{
    Inode* root = ramfs_alloc_inode();

    root->sb = sb;
    root->type = IT_DIR;
    root->dev = 0;
    root->nlinks = 0;
    root->refs = 1;
    root->size = 0;
    root->data = nullptr;
    root->ops = ramfs_dir_inode_ops;
    root->fops = ramfs_dir_file_ops;

    ramfs_link(root, ".", root);
    ramfs_link(root, "..", root);

    sb->root = root;
}

void ramfs_free_all(Superblock* sb)
{
    sb->root->put();

    for (int i = 0; i < INODE_TABLE_SIZE; i++)
    {
        Inode* inode = &inode_table.inodes[i];

        if (inode->refs && inode->sb == sb)
            inode->put();
    }

    kfree(sb->data);
}

result_ptr<Superblock> ramfs_create_sb(Filesystem* fs, Device* dev)
{
    Superblock* sb = (Superblock*)kmalloc(sizeof(Superblock));
    fs->num_sb++;

    sb->fs = fs;
    sb->dev = dev;
    sb->ops = { nullptr };

    ramfs_init(sb);

    return sb;
}

void ramfs_destroy_sb(Superblock* sb)
{
    ramfs_free_all(sb);

    sb->fs->num_sb--;
    kfree(sb);
}

int ramfs_create(Inode* dir, const char* name)
{
    Inode* inode = ramfs_alloc_inode();

    inode->sb = dir->sb;
    inode->type = IT_REG;
    inode->dev = 0;
    inode->nlinks = 0;
    inode->refs = 0;
    inode->size = 0;
    inode->data = nullptr;
    inode->ops = ramfs_reg_inode_ops;
    inode->fops = ramfs_reg_file_ops;

    ramfs_link(dir, name, inode);

    return 0;
}

int ramfs_mknod(Inode* dir, const char* name, uint32_t dev)
{
    Inode* inode = ramfs_alloc_inode();

    inode->sb = dir->sb;
    inode->type = IT_CDEV;
    inode->dev = dev;
    inode->nlinks = 0;
    inode->refs = 0;
    inode->size = 0;
    inode->data = nullptr;
    inode->ops = { nullptr };
    inode->fops = { nullptr };

    ramfs_link(dir, name, inode);

    return 0;
}

int ramfs_link(Inode* dir, const char* name, Inode* inode)
{
    RamDirent* dirents = (RamDirent*)dir->data;
    int count = dir->size / sizeof(RamDirent);

    for (int i = 0; i < count; i++)
    {
        if (dirents[i].name[0] == 0)
        {
            strcpy(dirents[i].name, name);
            dirents[i].ino = inode->ino;
            dirents[i].type = inode->type;
            inode->nlinks++;

            return 0;
        }
    }

    RamDirent newdirent;
    strcpy(newdirent.name, name);
    newdirent.ino = inode->ino;
    newdirent.type = inode->type;
    inode->nlinks++;

    ramfs_writei(dir, &newdirent, sizeof(RamDirent), dir->size);

    return 0;
}

int ramfs_unlink(Inode* dir, const char* name)
{
    RamDirent* dirents = (RamDirent*)dir->data;
    int count = dir->size / sizeof(RamDirent);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(dirents[i].name, name) == 0)
        {
            dirents[i].name[0] = 0;

            auto inode = dir->lookup(name);

            if (inode->nlinks > 0)
                inode->nlinks--;

            inode->put();

            return 0;
        }
    }

    return -ERR_NOT_FOUND;
}

int ramfs_mkdir(Inode* dir, const char* name)
{
    Inode* inode = ramfs_alloc_inode();

    inode->sb = dir->sb;
    inode->type = IT_DIR;
    inode->dev = 0;
    inode->nlinks = 0;
    inode->refs = 0;
    inode->size = 0;
    inode->data = nullptr;
    inode->ops = ramfs_dir_inode_ops;
    inode->fops = ramfs_dir_file_ops;

    ramfs_link(dir, name, inode);
    ramfs_link(inode, ".", inode);
    ramfs_link(inode, "..", dir);

    hexdump(inode->data, inode->size);

    return 0;
}

int ramfs_rmdir(Inode* dir, const char* name)
{
    if (dir->size <= 2 * sizeof(RamDirent))
        return -1;

    kfree(dir->data);
    dir->size = 0;

    ramfs_unlink(dir, name);

    return 0;
}

int ramfs_truncate(Inode* inode, size_t size)
{
    void* newdata = kmalloc(size);

    if (inode->data)
    {
        if (size < inode->size)
            memcpy(newdata, inode->data, size);
        else
        {
            memcpy(newdata, inode->data, inode->size);
            memset((char*)newdata + inode->size, 0, size - inode->size);
        }

        kfree(inode->data);
    }
    else
        memset(newdata, 0, size);

    inode->data = newdata;
    inode->size = size;

    return 0;
}

int ramfs_lookup(Inode* dir, const char* name, Inode* result)
{
    RamDirent* dirents = (RamDirent*)dir->data;
    int count = dir->size / sizeof(RamDirent);

    for (int i = 0; i < count; i++)
    {
        kprintf("strcmp(dirents[i].name = %s, name = %s) == 0\n", dirents[i].name, name);
        if (strcmp(dirents[i].name, name) == 0)
        {
            auto found = inode_table.find(dir->sb, dirents[i].ino);

            if (!found)
                return found.error();

            *result = *(found.ptr);

            return 0;
        }
    }

    return -ERR_NOT_FOUND;
}

int ramfs_sync(Inode* inode)
{
    if (inode->nlinks == 0)
        kfree(inode->data);

    return 0;
}

int ramfs_read(File* file, char* buf, size_t size, size_t offset)
{
    return ramfs_readi(file->inode, buf, size, offset);
}

int ramfs_write(File* file, const char* buf, size_t size, size_t offset)
{
    return ramfs_writei(file->inode, buf, size, offset);
}

int ramfs_iterate(File* file, void* buf, size_t size)
{
    if (file->offset >= file->inode->size)
        return 0;

    RamDirent* dirents = (RamDirent*)file->inode->data;
    RamDirent* dirent = &dirents[file->offset / sizeof(RamDirent)];

    size_t len = strlen(dirent->name);

    if (len + 1 > size)
        return -1;

    strcpy((char*)buf, dirent->name);

    return sizeof(RamDirent);
}

Filesystem ramfs =
{
    .name = "ramfs",
    .flags = 0,
    .create_sb = ramfs_create_sb,
    .destroy_sb = ramfs_destroy_sb
};

InodeOps ramfs_reg_inode_ops =
{
    .truncate = ramfs_truncate,
    .sync = ramfs_sync
};

InodeOps ramfs_dir_inode_ops =
{
    .create = ramfs_create,
    .mknod = ramfs_mknod,
    .link = ramfs_link,
    .unlink = ramfs_unlink,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .lookup = ramfs_lookup,
    .sync = ramfs_sync
};

FileOps ramfs_reg_file_ops =
{
    .read = ramfs_read,
    .write = ramfs_write,
    .seek = generic_seek
};

FileOps ramfs_dir_file_ops =
{
    .seek = generic_seek,
    .iterate = ramfs_iterate
};