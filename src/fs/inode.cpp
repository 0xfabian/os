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

Inode* Inode::get(const char* path)
{
    // this sould be running->fs.get_root() or .get_cwd()
    Inode* inode = root_mount->get_root();

    if (!inode)
        return nullptr;

    char* name;
    while ((name = path_read_next(path)))
    {
        // no need to search for .
        if (name[0] == '.' && name[1] == '\0')
            continue;

        // is this inode a mountpoint?
        Mount* mnt = Mount::find(inode);

        if (mnt)
        {
            // yes, so we switch to the mounts root
            inode->put();
            inode = mnt->get_root();
        }

        Inode* next = inode->lookup(name);

        // switch to the next
        inode->put();
        inode = next;

        // no entry found
        if (!next)
            return nullptr;

        // check for symlink here

        // path is not finished so next should be a directory
        if (*path != '\0' && !next->is_dir())
        {
            next->put();
            return nullptr;
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

Inode* Inode::lookup(const char* name)
{
    if (!ops.lookup)
    {
        kprintf(WARN "lookup(): called but not implemented\n");
        return nullptr;
    }

    Inode temp;

    if (ops.lookup(this, name, &temp) < 0)
        return nullptr;

    return inode_table.insert(&temp);
}

bool Inode::sync()
{
    if (ops.sync)
        return ops.sync(this);

    return false;
}

Inode* InodeTable::insert(Inode* inode)
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
    {
        kprintf(WARN "insert(): inode table is full\n");
        return nullptr;
    }

    *free = *inode;

    return free;
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

        if (inode->ops.lookup)
            kprintf("lookup ");

        if (inode->ops.sync)
            kprintf("sync ");

        kprintf("\n");
    }

    kprintf("}\n");
}