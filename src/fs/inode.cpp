#include <fs/inode.h>
#include <fs/mount.h>
#include <task.h>

InodeTable inode_table;

result_ptr<Inode> Inode::get(const char* path)
{
    Inode* inode;

    if (*path == '/' || !running)
        inode = root_mount->sb->root->get();
    else
        inode = running->cwd->get();

    // i think this should always be a valid pointer but just in case
    if (!inode)
        return -ERR_NOT_FOUND;

    char* name;
    while ((name = path_read_next(path)))
    {
        if (strcmp(name, ".") == 0)
            continue;

        // .. should always be a valid directory so we can do .ptr without checking
        if (strcmp(name, "..") == 0)
        {
            inode->put();

            Mount* mnt = Mount::find(inode->sb);

            // if this is a mount we need to lookup on the mountpoint inode
            // also the root has no mountpoint inode
            if (mnt && mnt->mp.inode)
                inode = mnt->mp.inode->lookup("..").ptr;
            else
                inode = inode->lookup("..").ptr;

            continue;
        }

        result_ptr<Inode> next = inode->lookup(name);

        if (!next)
            return next.error();

        // switch to the next
        inode->put();
        inode = next.ptr;

        // check for symlink here

        // path is not finished so next should be a directory
        if (*path != '\0' && !next->is_dir())
        {
            next->put();
            return -ERR_NOT_DIR;
        }

        // is this inode a mountpoint?
        Mount* mnt = Mount::find(inode);

        if (mnt)
        {
            // yes, so we switch to the mounts root
            inode->put();
            inode = mnt->sb->root->get();
        }
    }

    return inode;
}

Inode* Inode::get()
{
    refs++;
    return this;
}

void Inode::put()
{
    refs--;

    if (refs > 0)
        return;

    // should probably sync only if dirty
    sync();

    if ((flags & IF_PERSISTENT) == 0)
        flags &= ~IF_ALLOC;
}

bool Inode::is_reg()
{
    return (type & IT_REG) == IT_REG;
}

bool Inode::is_dir()
{
    return (type & IT_DIR) == IT_DIR;
}

bool Inode::is_device()
{
    return is_char_device() || is_block_device();
}

bool Inode::is_block_device()
{
    return (type & IT_BDEV) == IT_BDEV;
}

bool Inode::is_char_device()
{
    return (type & IT_CDEV) == IT_CDEV;
}

int Inode::create(const char* name)
{
    if (!ops.create)
        return -ERR_NOT_IMPL;

    return ops.create(this, name);
}

int Inode::mknod(const char* name, u32 dev)
{
    if (!ops.mknod)
        return -ERR_NOT_IMPL;

    return ops.mknod(this, name, dev);
}

int Inode::link(const char* name, Inode* inode)
{
    if (!ops.link)
        return -ERR_NOT_IMPL;

    return ops.link(this, name, inode);
}

int Inode::unlink(const char* name)
{
    if (!ops.unlink)
        return -ERR_NOT_IMPL;

    return ops.unlink(this, name);
}

int Inode::mkdir(const char* name)
{
    if (!ops.mkdir)
        return -ERR_NOT_IMPL;

    return ops.mkdir(this, name);
}

int Inode::rmdir(const char* name)
{
    if (!ops.rmdir)
        return -ERR_NOT_IMPL;

    return ops.rmdir(this, name);
}

int Inode::truncate(usize size)
{
    if (!ops.truncate)
        return -ERR_NOT_IMPL;

    return ops.truncate(this, size);
}

result_ptr<Inode> Inode::lookup(const char* name)
{
    if (!ops.lookup)
        return -ERR_NOT_IMPL;

    Inode temp;

    int err = ops.lookup(this, name, &temp);

    if (err)
        return err;

    return inode_table.insert(&temp);
}

int Inode::sync()
{
    if (!ops.sync)
        return 0;

    return ops.sync(this);
}

result_ptr<Inode> InodeTable::insert(Inode* inode)
{
    // pointer to a free entry in the table
    Inode* free = nullptr;

    for (Inode* i = &inodes[0]; i < &inodes[INODE_TABLE_SIZE]; i++)
    {
        if ((i->flags & IF_ALLOC) == 0)
        {
            if (!free)
                free = i;

            continue;
        }

        // the inode is already in the table
        if (i->sb == inode->sb && i->ino == inode->ino)
        {
            i->refs++;
            return i;
        }
    }

    // inode is not in the table so we create a new one

    if (!free)
        return -ERR_INODE_TABLE_FULL;

    *free = *inode;

    free->refs = 1;
    free->flags |= IF_ALLOC;

    return free;
}

result_ptr<Inode> InodeTable::find(Superblock* sb, u64 ino)
{
    for (Inode* i = &inodes[0]; i < &inodes[INODE_TABLE_SIZE]; i++)
        if ((i->flags & IF_ALLOC) && i->sb == sb && i->ino == ino)
            return i;

    return -ERR_NOT_FOUND;
}

usize InodeTable::get_sb_refs(Superblock* sb)
{
    usize refs = 0;

    for (Inode* i = &inodes[0]; i < &inodes[INODE_TABLE_SIZE]; i++)
        if ((i->flags & IF_ALLOC) && i->sb == sb)
            refs += i->refs;

    return refs;
}

void InodeTable::debug()
{
    kprintf("Inode table:\n{\n");

    for (int i = 0; i < INODE_TABLE_SIZE; i++)
    {
        Inode* inode = &inodes[i];

        if ((inode->flags & IF_ALLOC) == 0)
            continue;

        kprintf("    %d: sb=%a ino=%lu type=%x size=%lu refs=%d nlinks=%d flags=%hhx ", i, inode->sb, inode->ino, inode->type, inode->size, inode->refs, inode->nlinks, inode->flags);

        if (inode->ops.create)
            kprintf("create ");

        if (inode->ops.mknod)
            kprintf("mknod ");

        if (inode->ops.link)
            kprintf("link ");

        if (inode->ops.unlink)
            kprintf("unlink ");

        if (inode->ops.mkdir)
            kprintf("mkdir ");

        if (inode->ops.rmdir)
            kprintf("rmdir ");

        if (inode->ops.truncate)
            kprintf("truncate ");

        if (inode->ops.lookup)
            kprintf("lookup ");

        if (inode->ops.sync)
            kprintf("sync ");

        kprintf("\n");
    }

    kprintf("}\n");
}