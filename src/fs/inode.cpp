#include <fs/inode.h>
#include <fs/mount.h>

InodeTable inode_table;

#define PATH_MAX 256
char path_read_data[PATH_MAX];

// hacky path reading function
char* path_read_next(const char*& ptr)
{
    while (*ptr == '/')
        ptr++;

    if (*ptr == '\0')
        return nullptr;

    char* output = path_read_data;
    size_t len = 0;

    while (*ptr != '/' && *ptr != '\0')
    {
        if (len >= PATH_MAX - 1)
        {
            kprintf(WARN "path_read_next(): path too long\n");
            break;
        }

        output[len++] = *ptr++;
    }

    output[len] = '\0';

    return path_read_data;
}

result_ptr<Inode> Inode::get(const char* path)
{
    // this sould be running->fs.get_root() or .get_cwd()
    Inode* inode = root_mount->get_root();

    if (!inode)
        return -ERR_NOT_FOUND;

    char* name;
    while ((name = path_read_next(path)))
    {
        if (name[0] == '.' && name[1] == '\0')
            continue;

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
            inode = mnt->get_root();
        }
    }

    return inode;
}

void Inode::put()
{
    refs--;

    if (refs > 0)
        return;

    // should probably sync only if dirty
    sync();
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

// this should handle only name, not path
// and the name should already be valid
int Inode::mkdir(const char* name)
{
    if (!ops.mkdir)
        return -ERR_NOT_IMPL;

    if (!is_dir())
        return -ERR_NOT_DIR;

    // should check for existance with lookup

    return ops.mkdir(this, name);
}

result_ptr<Inode> Inode::lookup(const char* name)
{
    if (!ops.lookup)
        return -ERR_NOT_IMPL;

    if (!is_dir())
        return -ERR_NOT_DIR;

    Inode temp;

    int err = ops.lookup(this, name, &temp);

    if (err < 0)
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
        if (i->refs == 0)
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

    return free;
}

size_t InodeTable::get_sb_refs(Superblock* sb)
{
    size_t refs = 0;

    for (Inode* i = &inodes[0]; i < &inodes[INODE_TABLE_SIZE]; i++)
        if (i->refs && i->sb == sb)
            refs += i->refs;

    return refs;
}

void InodeTable::debug()
{
    kprintf("Inode table:\n{\n");

    for (int i = 0; i < INODE_TABLE_SIZE; i++)
    {
        Inode* inode = &inodes[i];

        if (inode->refs == 0)
            continue;

        kprintf("    %d: sb=%a ino=%lu type=%x size=%lu refs=%d ", i, inode->sb, inode->ino, inode->type, inode->size, inode->refs);

        if (inode->ops.mkdir)
            kprintf("mkdir ");

        if (inode->ops.lookup)
            kprintf("lookup ");

        if (inode->ops.sync)
            kprintf("sync ");

        kprintf("\n");
    }

    kprintf("}\n");
}