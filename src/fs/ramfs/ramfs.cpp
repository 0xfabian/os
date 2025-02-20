#include <fs/ramfs/ramfs.h>

#define RAMFS_MAX_FILES 256

struct RamDirent
{
    char name[256];
    uint64_t ino;
    uint32_t type;
};

struct RamInode
{
    uint64_t ino;
    uint32_t type;
    size_t size;
    int nlinks;
    void* data;
};

void ramfs_init(Superblock* sb)
{
    sb->data = kmalloc(RAMFS_MAX_FILES * sizeof(RamInode));

    RamInode* inodes = (RamInode*)sb->data;

    for (int i = 0; i < RAMFS_MAX_FILES; i++)
    {
        inodes[i].ino = i + 1;
        inodes[i].type = 0;
        inodes[i].size = 0;
        inodes[i].nlinks = 0;
        inodes[i].data = nullptr;
    }

    RamInode* root = &inodes[0];
    root->ino = 1;
    root->type = IT_DIR;
    root->size = 2 * sizeof(RamDirent);
    root->nlinks = 1;
    root->data = kmalloc(root->size);

    RamDirent* dirents = (RamDirent*)root->data;

    dirents[0].ino = 1;
    root->nlinks++;
    strcpy(dirents[0].name, ".");
    dirents[0].type = IT_DIR;

    dirents[1].ino = 1;
    root->nlinks++;
    strcpy(dirents[1].name, "..");
    dirents[1].type = IT_DIR;

    Inode _root;
    _root.sb = sb;
    _root.ino = root->ino;
    _root.type = root->type;
    _root.size = root->size;
    _root.refs = 1;
    _root.ops = { ramfs_mkdir, ramfs_lookup, nullptr };
    _root.fops = { nullptr, nullptr, ramfs_read, nullptr, generic_seek, ramfs_iterate };

    sb->root = inode_table.insert(&_root);
}

void ramfs_free_all(Superblock* sb)
{
    sb->root->put();

    RamInode* inodes = (RamInode*)sb->data;

    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (inodes[i].nlinks && inodes[i].data)
            kfree(inodes[i].data);

    kfree(sb->data);
}

RamInode* ramfs_alloc_inode(Superblock* sb)
{
    RamInode* inodes = (RamInode*)sb->data;

    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (inodes[i].nlinks == 0)
            return &inodes[i];

    return nullptr;
}

RamInode* from_inode(Inode* inode)
{
    RamInode* inodes = (RamInode*)inode->sb->data;

    return &inodes[inode->ino - 1];
}

RamInode* from_ino(Superblock* sb, uint64_t ino)
{
    RamInode* inodes = (RamInode*)sb->data;

    return &inodes[ino - 1];
}

Inode to_inode(RamInode* inode, Inode* like)
{
    Inode ret;

    ret.sb = like->sb;
    ret.ino = inode->ino;
    ret.type = inode->type;
    ret.size = inode->size;
    ret.refs = 1;
    ret.ops = like->ops;
    ret.fops = like->fops;

    return ret;
}

int add_dirent(Inode* _dir, const char* name, RamInode* inode)
{
    RamInode* dir = from_inode(_dir);

    RamDirent* newdirent = (RamDirent*)kmalloc(dir->size + sizeof(RamDirent));
    memcpy(newdirent, dir->data, dir->size);

    RamDirent* last = newdirent + dir->size / sizeof(RamDirent);
    strcpy(last->name, name);
    last->ino = inode->ino;
    inode->nlinks++;
    last->type = inode->type;

    kfree(dir->data);
    dir->data = newdirent;
    dir->size += sizeof(RamDirent);
    _dir->size = dir->size;

    return 0;
}

int ramfs_mkdir(Inode* _dir, const char* name)
{
    RamInode* newdir = ramfs_alloc_inode(_dir->sb);

    if (!newdir)
        return -1;

    newdir->type = IT_DIR;

    if (add_dirent(_dir, name, newdir) < 0)
        return -1;

    // nlinks should be 1 so now its allocated

    newdir->size = 2 * sizeof(RamDirent);
    newdir->data = kmalloc(newdir->size);

    RamDirent* dirents = (RamDirent*)newdir->data;

    dirents[0].ino = newdir->ino;
    newdir->nlinks++;
    strcpy(dirents[0].name, ".");
    dirents[0].type = IT_DIR;

    RamInode* dir = from_inode(_dir);

    dirents[1].ino = dir->ino;
    dir->nlinks++;
    strcpy(dirents[1].name, "..");
    dirents[1].type = IT_DIR;

    return 0;
}

int ramfs_lookup(Inode* _dir, const char* name, Inode* result)
{
    RamInode* dir = from_inode(_dir);

    RamDirent* dirents = (RamDirent*)dir->data;
    int count = dir->size / sizeof(RamDirent);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(dirents[i].name, name) == 0)
        {
            RamInode* found = from_ino(_dir->sb, dirents[i].ino);

            *result = to_inode(found, _dir);

            return 0;
        }
    }

    // not found

    return -1;
}

int ramfs_read(File* file, char* buf, size_t size, size_t offset)
{
    if (size == 0)
        return 0;

    RamInode* inode = from_inode(file->inode);

    // again it should never be null

    if (offset >= inode->size)
        return 0;

    size_t remaining = inode->size - offset;

    if (size > remaining)
        size = remaining;

    memcpy(buf, (char*)inode->data + offset, size);

    return size;
}

int ramfs_iterate(File* file, void* buf, size_t size)
{
    if (file->offset >= file->inode->size)
        return 0;

    RamInode* dir = from_inode(file->inode);

    RamDirent* dirents = (RamDirent*)dir->data;
    RamDirent* dirent = &dirents[file->offset / sizeof(RamDirent)];

    size_t len = strlen(dirent->name);

    if (len + 1 > size)
        return -1;

    strcpy((char*)buf, dirent->name);

    return sizeof(RamDirent);
}

Superblock* ramfs_create_sb(Filesystem* fs, Device* dev)
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

Filesystem ramfs =
{
    .name = "ramfs",
    .flags = 0,
    .create_sb = ramfs_create_sb,
    .destroy_sb = ramfs_destroy_sb
};