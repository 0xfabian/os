#include <fs/ramfs/ramfs.h>

#define RAMFS_MAX_FILES 256

struct RamfsDirent
{
    char name[256];
    uint64_t ino;
    uint32_t type;
};

struct RamfsInode
{
    uint64_t ino;
    uint32_t type;
    size_t size;
    int nlinks;
    char* data;
};

RamfsInode* from_inode(Inode* inode)
{
    RamfsInode* inodes = (RamfsInode*)inode->sb->data;

    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (inodes[i].ino == inode->ino)
            return &inodes[i];

    return nullptr;
}

Inode to_inode(Inode* from, RamfsInode* ramfs_inode)
{
    Inode inode;

    inode.sb = from->sb;
    inode.ino = ramfs_inode->ino;
    inode.type = ramfs_inode->type;
    inode.size = ramfs_inode->size;
    inode.refs = 1;
    inode.ops = from->ops;
    inode.fops = from->fops;

    return inode;
}

int ramfs_lookup(Inode* inode, const char* name, Inode* result)
{
    RamfsInode* ramfs_inode = from_inode(inode);

    if (!ramfs_inode)
        return -1;

    if (ramfs_inode->type != IT_DIR)
        return -1;

    RamfsDirent* dirents = (RamfsDirent*)ramfs_inode->data;
    int count = ramfs_inode->size / sizeof(RamfsDirent);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(dirents[i].name, name) == 0)
        {
            Inode temp;
            temp.sb = inode->sb;
            temp.ino = dirents[i].ino;

            RamfsInode* ret = from_inode(&temp);

            *result = to_inode(inode, ret);

            return 0;
        }
    }

    return -1;
}

int ramfs_read(File* file, char* buf, size_t size, size_t offset)
{
    RamfsInode* ramfs_inode = from_inode(file->inode);

    // it should never be null

    if (offset >= ramfs_inode->size)
        return 0;

    size_t remaining = ramfs_inode->size - offset;

    if (size > remaining)
        size = remaining;

    memcpy(buf, ramfs_inode->data + offset, size);

    return size;
}

Superblock* ramfs_create_sb(Filesystem* fs, Device* dev)
{
    Superblock* sb = (Superblock*)kmalloc(sizeof(Superblock));

    sb->fs = fs;
    sb->dev = dev;
    sb->data = kmalloc(sizeof(RamfsInode) * RAMFS_MAX_FILES);

    RamfsInode* inodes = (RamfsInode*)sb->data;

    for (int i = 0; i < RAMFS_MAX_FILES; i++)
    {
        inodes[i].ino = 0;
        inodes[i].type = 0;
        inodes[i].size = 0;
        inodes[i].nlinks = 0;
        inodes[i].data = nullptr;
    }

    RamfsInode* root = &inodes[0];
    root->ino = 1;
    root->type = IT_DIR;
    root->size = 3 * sizeof(RamfsDirent);
    root->nlinks = 1;
    root->data = (char*)kmalloc(root->size);

    RamfsDirent* dirents = (RamfsDirent*)root->data;

    dirents[0].ino = 1;
    strcpy(dirents[0].name, ".");
    dirents[0].type = IT_DIR;

    dirents[1].ino = 1;
    strcpy(dirents[1].name, "..");
    dirents[1].type = IT_DIR;

    dirents[2].ino = 2;
    strcpy(dirents[2].name, "hello.txt");
    dirents[2].type = IT_REG;

    RamfsInode* hello = &inodes[1];
    hello->ino = 2;
    hello->type = IT_REG;
    hello->size = 11;
    hello->nlinks = 1;
    hello->data = (char*)kmalloc(hello->size);
    memcpy(hello->data, "Hello, OS!\n", hello->size);

    Inode root_inode;
    root_inode.sb = sb;
    root_inode.ino = 1;
    root_inode.type = IT_DIR;
    root_inode.size = root->size;
    root_inode.refs = 1;
    root_inode.ops = { ramfs_lookup, nullptr };
    root_inode.fops = { nullptr, nullptr, ramfs_read, nullptr, nullptr };

    sb->root = inode_table.insert(&root_inode);

    fs->num_sb++;

    return sb;
}

void ramfs_destroy_sb(Superblock* sb)
{
    sb->root->put();

    RamfsInode* inodes = (RamfsInode*)sb->data;
    for (int i = 0; i < RAMFS_MAX_FILES; i++)
        if (inodes[i].data)
            kfree(inodes[i].data);

    kfree(sb->data);

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