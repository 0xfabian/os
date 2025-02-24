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

        if (!newdata)
            return -ERR_NO_MEM;

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

        if (inode->flags & IF_ALLOC)
            continue;

        memset(inode, 0, sizeof(Inode));

        inode->ino = i;
        inode->flags |= IF_ALLOC | IF_PERSISTENT;

        return inode;
    }

    return nullptr;
}

void ramfs_free_inode(Inode* inode)
{
    if (inode->data)
        kfree(inode->data);

    inode->flags &= ~IF_ALLOC;
}

result_ptr<Superblock> ramfs_create_sb(Filesystem* fs, Device* dev)
{
    Superblock* sb = (Superblock*)kmalloc(sizeof(Superblock));

    sb->fs = fs;
    sb->dev = dev;

    Inode* root = ramfs_alloc_inode();

    root->sb = sb;
    root->type = IT_DIR;
    root->refs = 1; // referenced by sb
    root->ops = ramfs_dir_inode_ops;
    root->fops = ramfs_dir_file_ops;

    // these could fail
    ramfs_link(root, ".", root);
    ramfs_link(root, "..", root);

    sb->root = root;
    sb->data = nullptr;
    sb->ops = { nullptr };

    return sb;
}

void ramfs_destroy_sb(Superblock* sb)
{
    sb->root->put();

    for (int i = 0; i < INODE_TABLE_SIZE; i++)
    {
        Inode* inode = &inode_table.inodes[i];

        if ((inode->flags & IF_ALLOC) && inode->sb == sb)
            ramfs_free_inode(inode);
    }

    kfree(sb);
}

int ramfs_create(Inode* dir, const char* name)
{
    Inode* inode = ramfs_alloc_inode();

    inode->sb = dir->sb;
    inode->type = IT_REG;
    inode->ops = ramfs_reg_inode_ops;
    inode->fops = ramfs_reg_file_ops;

    int err = ramfs_link(dir, name, inode);

    if (err)
    {
        inode->flags &= ~IF_ALLOC;
        return err;
    }

    return 0;
}

int ramfs_mknod(Inode* dir, const char* name, uint32_t dev)
{
    Inode* inode = ramfs_alloc_inode();

    inode->sb = dir->sb;
    inode->type = IT_CDEV;
    inode->dev = dev;

    int err = ramfs_link(dir, name, inode);

    if (err)
    {
        inode->flags &= ~IF_ALLOC;
        return err;
    }

    return 0;
}

RamDirent* ramfs_find_dirent(Inode* dir, const char* name)
{
    RamDirent* dirents = (RamDirent*)dir->data;
    int count = dir->size / sizeof(RamDirent);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(dirents[i].name, name) == 0)
            return &dirents[i];
    }

    return nullptr;
}

int ramfs_link(Inode* dir, const char* name, Inode* inode)
{
    RamDirent* free = ramfs_find_dirent(dir, "");

    if (free)
    {
        strcpy(free->name, name);
        free->ino = inode->ino;
        free->type = inode->type;
        inode->nlinks++;

        return 0;
    }

    RamDirent dirent;
    strcpy(dirent.name, name);
    dirent.ino = inode->ino;
    dirent.type = inode->type;
    inode->nlinks++;

    return ramfs_writei(dir, &dirent, sizeof(RamDirent), dir->size);
}

int ramfs_unlink(Inode* dir, const char* name)
{
    RamDirent* dirent = ramfs_find_dirent(dir, name);

    if (!dirent)
        return -ERR_NOT_FOUND;

    dirent->name[0] = 0;

    // this should always be found
    auto inode = inode_table.find(dir->sb, dirent->ino);

    // should be > 0 since we found it
    inode->nlinks--;

    if (inode->nlinks == 0 && inode->refs == 0)
        ramfs_free_inode(inode.ptr);

    return 0;
}

int ramfs_mkdir(Inode* dir, const char* name)
{
    Inode* inode = ramfs_alloc_inode();

    inode->sb = dir->sb;
    inode->type = IT_DIR;
    inode->ops = ramfs_dir_inode_ops;
    inode->fops = ramfs_dir_file_ops;

    int err = ramfs_link(dir, name, inode);

    if (!err)
        ramfs_link(inode, ".", inode);

    if (!err)
        ramfs_link(inode, "..", dir);

    if (err)
    {
        inode->flags &= ~IF_ALLOC;
        return err;
    }

    return 0;
}

int ramfs_rmdir(Inode* dir, const char* name)
{
    RamDirent* dirent = ramfs_find_dirent(dir, name);

    if (!dirent)
        return -ERR_NOT_FOUND;

    if (dirent->type != IT_DIR)
        return -ERR_NOT_DIR;

    auto inode = inode_table.find(dir->sb, dirent->ino);

    if (inode->nlinks > 2)
        return -ERR_DIR_NOT_EMPTY;

    // directories dont have hard links other than . or ..

    dirent->name[0] = 0;
    dir->nlinks--;

    if (inode->refs)
        inode->nlinks = 0; // removed later by sync
    else
        ramfs_free_inode(inode.ptr);

    return 0;
}

int ramfs_truncate(Inode* inode, size_t size)
{
    if (size == 0)
    {
        if (inode->data)
            kfree(inode->data);

        inode->data = nullptr;
        inode->size = 0;

        return 0;
    }

    void* newdata = kmalloc(size);

    if (!newdata)
        return -ERR_NO_MEM;

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
    RamDirent* dirent = ramfs_find_dirent(dir, name);

    if (!dirent)
        return -ERR_NOT_FOUND;

    auto found = inode_table.find(dir->sb, dirent->ino);

    *result = *(found.ptr);

    return 0;
}

int ramfs_sync(Inode* inode)
{
    // dont need to sync anything for ramfs inode

    if (inode->nlinks == 0)
        ramfs_free_inode(inode);

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
    int count = file->inode->size / sizeof(RamDirent);
    int index = file->offset / sizeof(RamDirent);

    RamDirent* dirent = &dirents[index];

    size_t len = strlen(dirent->name);

    if (len + 1 > size)
        return -1;

    strcpy((char*)buf, dirent->name);

    int skip = 1;

    for (int i = index + 1; i < count; i++)
    {
        if (dirents[i].name[0] != 0)
            break;

        skip++;
    }

    return skip * sizeof(RamDirent);
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