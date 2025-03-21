#include <fs/file.h>

FileTable file_table;

result_ptr<File> File::open(const char* path, u32 flags)
{
    if (flags & O_EXCL && !(flags & O_CREAT))
        return -ERR_BAD_ARG;

    auto inode = Inode::get(path);

    if (!inode)
    {
        if (!(flags & O_CREAT))
            return inode.error();

        const char* name = basename(path);

        if (!name)
            return -ERR_BAD_ARG;

        auto dir = Inode::get(dirname(path));

        if (!dir)
            return dir.error();

        int err = dir->create(name);
        dir->put();

        if (err)
            return err;

        inode = Inode::get(path);

        if (!inode)
            return inode.error();
    }
    else if (flags & O_EXCL)
    {
        inode->put();
        return -ERR_EXISTS;
    }

    if (flags & O_TRUNC)
    {
        int err = inode->truncate(0);

        if (err)
        {
            inode->put();
            return err;
        }
    }

    auto file = file_table.alloc();

    if (!file)
    {
        inode->put();
        return file.error();
    }

    file->offset = 0;
    file->flags = flags;
    file->refs = 1;
    file->inode = inode.ptr;

    if (inode->is_device())
    {
        fbterm.give_fops(&file->ops);

        // Device* dev = inode->get_device();

        // // more care here
        // if (!dev)
        // {
        //     file->refs = 0;
        //     inode->put();
        //     return nullptr;
        // }

        // file->ops = dev->ops;

        // // call the driver open function, should probably check for failure
        // if (file->ops.open)
        //     file->ops.open(file);

        // kprintf(WARN "open(): device files not implemented\n");

        // file->refs = 0;
        // inode->put();

        // return -ERR_NOT_IMPL;
    }
    else
        file->ops = inode->fops;

    return file;
}

int File::close()
{
    // decrement references
    refs--;

    if (refs > 0)
        return 0;

    // at this point we dont have any references

    // wake up process if the file is a pipe
    // if (is_pipe())
    //     wakeup_process();

    // if the file is a device call the devices close function
    if (inode->is_device() && ops.close)
        ops.close(this);

    // now the file is gone so we should release the inode
    inode->put();

    return 0;
}

isize File::read(void* buf, usize size)
{
    if (!ops.read)
        return -ERR_NOT_IMPL;

    isize ret = ops.read(this, buf, size, offset);

    if (ret < 0)
        return ret;

    offset += ret;

    return ret;
}

isize File::write(const void* buf, usize size)
{
    if (!ops.write)
        return -ERR_NOT_IMPL;

    isize ret = ops.write(this, buf, size, offset);

    if (ret < 0)
        return ret;

    offset += ret;

    return ret;
}

isize File::pread(void* buf, usize size, usize offset)
{
    if (!ops.read)
        return -ERR_NOT_IMPL;

    return ops.read(this, buf, size, offset);
}

isize File::pwrite(const void* buf, usize size, usize offset)
{
    if (!ops.write)
        return -ERR_NOT_IMPL;

    return ops.write(this, buf, size, offset);
}

usize File::seek(usize offset, int whence)
{
    if (!ops.seek)
        return -ERR_NOT_IMPL;

    return ops.seek(this, offset, whence);
}

int File::iterate(void* buf, usize size)
{
    if (!ops.iterate)
        return -ERR_NOT_IMPL;

    if (!inode->is_dir())
        return -ERR_NOT_DIR;

    int ret = ops.iterate(this, buf, size);

    if (ret < 0)
        return ret;

    offset += ret;

    return ret;
}

int File::ioctl(int cmd, void* arg)
{
    if (!ops.ioctl)
        return -ERR_NOT_IMPL;

    return ops.ioctl(this, cmd, arg);
}

File* File::dup()
{
    refs++;
    return this;
}

result_ptr<File> FileTable::alloc()
{
    for (File* file = &files[0]; file < &files[FILE_TABLE_SIZE]; file++)
        if (file->refs == 0)
            return file;

    return -ERR_FILE_TABLE_FULL;
}

void FileTable::debug()
{
    kprintf("\e[7m %-8s %-12s %-8s %-16s %-8s %-40s \e[27m\n", "ENTRY", "OFFSET", "INODE", "FLAGS", "REFS", "OPERATIONS");

    for (int i = 0; i < FILE_TABLE_SIZE; i++)
    {
        File* file = &files[i];

        if (file->refs == 0)
            continue;

        kprintf(" %-8d %-12lu %-8lu %-16x %-8d ", i + 1, file->offset, file->inode - inode_table.inodes + 1, file->flags, file->refs);

        if (file->ops.open)
            kprintf("open ");

        if (file->ops.close)
            kprintf("close ");

        if (file->ops.read)
            kprintf("read ");

        if (file->ops.write)
            kprintf("write ");

        if (file->ops.seek)
            kprintf("seek ");

        if (file->ops.iterate)
            kprintf("iterate ");

        if (file->ops.ioctl)
            kprintf("ioctl ");

        kprintf("\n");
    }
}

usize generic_seek(File* file, usize offset, int whence)
{
    switch (whence)
    {
    case SEEK_SET:
        file->offset = offset;
        break;

    case SEEK_CUR:
        file->offset += offset;
        break;

    case SEEK_END:
        file->offset = file->inode->size + offset;
        break;

    default:
        return -ERR_BAD_ARG;
    }

    return file->offset;
}