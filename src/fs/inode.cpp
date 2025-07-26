#include <fs/inode.h>
#include <fs/mount.h>
#include <task.h>

InodeTable inode_table;

result_ptr<Inode> namei(Inode* dir, const char* path, bool follow, usize* links_seen)
{
    Inode* inode = dir->get();

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
            // if this inode is the root of it's superblock
            // we need to lookup .. on the mountpoint inode instead
            if (inode == inode->sb->root)
            {
                Mount* mnt = Mount::find(inode->sb);

                // mnt could be root_mount who doesn't have a mountpoint inode
                if (mnt->mp.inode)
                {
                    inode->put();
                    inode = mnt->mp.inode->lookup("..").ptr;

                    continue;
                }
            }

            // at this point just do regular lookup

            Inode* next = inode->lookup("..").ptr;

            inode->put();
            inode = next;

            continue;
        }

        result_ptr<Inode> next = inode->lookup(name);

        if (!next)
        {
            inode->put();
            return next.error();
        }

        // check for symlink here
        while (next->is_link() && (*path != '\0' || follow))
        {
            if (*links_seen > 8)
            {
                next->put();
                inode->put();
                return -ERR_TOO_MANY_LINKS;
            }

            char target_path[256];
            if (next->size > 255)
            {
                next->put();
                inode->put();
                return -ERR_NO_MEM;
            }

            // hack, only works for ramfs
            File dummy_file;
            dummy_file.inode = next.ptr;
            isize bytes = next->fops.read(&dummy_file, target_path, next->size, 0);

            if (bytes < 0)
            {
                next->put();
                inode->put();
                return bytes;
            }

            target_path[bytes] = '\0';

            Inode* root = target_path[0] == '/' ? root_mount->sb->root : inode;
            (*links_seen)++;
            result_ptr<Inode> target = namei(root, target_path, follow, links_seen);

            if (!target)
            {
                next->put();
                inode->put();
                return target.error();
            }

            next->put();
            next = target;
        }

        // switch to the next
        inode->put();
        inode = next.ptr;

        // path is not finished so next should be a directory
        if (*path != '\0' && !inode->is_dir())
        {
            inode->put();
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

result_ptr<Inode> Inode::get(const char* path, bool follow)
{
    Inode* inode;

    if (*path == '/' || !running)
        inode = root_mount->sb->root;
    else
        inode = running->cwd;

    usize links_seen = 0;
    return namei(inode, path, follow, &links_seen);
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
    return (mode & IT_MASK) == IT_REG;
}

bool Inode::is_dir()
{
    return (mode & IT_MASK) == IT_DIR;
}

bool Inode::is_device()
{
    return is_char_device() || is_block_device();
}

bool Inode::is_block_device()
{
    return (mode & IT_MASK) == IT_BDEV;
}

bool Inode::is_char_device()
{
    return (mode & IT_MASK) == IT_CDEV;
}

bool Inode::is_link()
{
    return (mode & IT_MASK) == IT_LINK;
}

bool Inode::is_readable()
{
    return (mode & IP_MASK) & IP_R;
}

bool Inode::is_writable()
{
    return (mode & IP_MASK) & IP_W;
}

bool Inode::is_executable()
{
    return (mode & IP_MASK) & IP_X;
}

int Inode::create(const char* name, u32 mode)
{
    if (!ops.create)
        return -ERR_NOT_IMPL;

    return ops.create(this, name, (mode & IP_MASK) | IT_REG);
}

int Inode::symlink(const char* target, const char* path)
{
    if (!ops.create)
        return -ERR_NOT_IMPL;

    int ret = ops.create(this, basename(path), IT_LINK | IP_RWX);

    if (ret != 0)
        return ret;

    auto file = File::open(path, O_WRONLY | O_NOFOLLOW, 0);

    if (!file)
        return file.error();

    isize bytes = file->write(target, strlen(target));
    file->close();

    if (bytes < 0)
        return bytes;

    return 0;
}

int Inode::mknod(const char* name, u32 mode, u32 dev)
{
    if (!ops.mknod)
        return -ERR_NOT_IMPL;

    u32 type = mode & IT_MASK;

    if (type != IT_CDEV && type != IT_BDEV)
        return -ERR_BAD_ARG;

    return ops.mknod(this, name, mode, dev);
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

int Inode::fill_stat(stat* buf)
{
    buf->st_dev = 0;
    buf->st_ino = ino;
    buf->st_nlink = nlinks;
    buf->st_mode = mode;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_rdev = dev;
    buf->st_size = size;
    buf->st_blksize = 4096;
    buf->st_blocks = (size + 511) / 512;

    buf->st_atime = 0;
    buf->st_mtime = 0;
    buf->st_ctime = 0;

    buf->st_atimensec = 0;
    buf->st_mtimensec = 0;
    buf->st_ctimensec = 0;

    return 0;
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
    kprintf("\e[7m %-8s %-18s    %-8s %-10s    %-8s %-8s %-8s %-8s %-57s \e[27m\n", "ENTRY", "SUPERBLOCK", "INO", "MODE", "SIZE", "REFS", "NLINKS", "FLAGS", "OPERATIONS");

    for (int i = 0; i < INODE_TABLE_SIZE; i++)
    {
        Inode* inode = &inodes[i];

        if ((inode->flags & IF_ALLOC) == 0)
            continue;

        char mode_str[11];
        static const char* type_str = "?pc d b - l s   ";

        mode_str[0] = type_str[(inode->mode & IT_MASK) >> 12];
        mode_str[1] = (inode->mode & IP_R_U) ? 'r' : '-';
        mode_str[2] = (inode->mode & IP_W_U) ? 'w' : '-';
        mode_str[3] = (inode->mode & IP_X_U) ? 'x' : '-';
        mode_str[4] = (inode->mode & IP_R_G) ? 'r' : '-';
        mode_str[5] = (inode->mode & IP_W_G) ? 'w' : '-';
        mode_str[6] = (inode->mode & IP_X_G) ? 'x' : '-';
        mode_str[7] = (inode->mode & IP_R_O) ? 'r' : '-';
        mode_str[8] = (inode->mode & IP_W_O) ? 'w' : '-';
        mode_str[9] = (inode->mode & IP_X_O) ? 'x' : '-';
        mode_str[10] = '\0';

        kprintf(" %-8d %p    %-8lu %s    %-8lu %-8d %-8d %-8hhx ", i + 1, inode->sb, inode->ino, mode_str, inode->size, inode->refs, inode->nlinks, inode->flags);

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
}
