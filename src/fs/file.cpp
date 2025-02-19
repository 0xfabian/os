#include <fs/file.h>

FileTable file_table;

File* File::open(const char* path, uint32_t flags)
{
    // Inode* inode = Inode::get(path);

    // // should probably check if is O_CREAT
    // if (!inode)
    //     return nullptr;

    // File* file = file_table.alloc();

    // if (!file)
    // {
    //     inode->put();
    //     return nullptr;
    // }

    // file->offset = 0;
    // file->flags = flags;
    // file->refs = 1;
    // file->inode = inode;

    // if (inode->is_device())
    // {
    //     Device* dev = inode->get_device();

    //     // more care here
    //     if (!dev)
    //     {
    //         file->refs = 0;
    //         inode->put();
    //         return nullptr;
    //     }

    //     file->ops = dev->ops;

    //     // call the driver open function, should probably check for failure
    //     if (file->ops.open)
    //         file->ops.open(file);
    // }
    // else
    //     file->ops = inode->fops;

    // return file;

    File* file = file_table.alloc();

    if (!file)
        return nullptr;

    file->offset = 0;
    file->inode = nullptr;
    file->flags = 0;
    file->refs = 1;
    file->ops = { nullptr, nullptr, nullptr, nullptr, nullptr };

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
    // if (inode->is_device() && ops.close)
    //     ops.close(this);

    // now the file is gone so we should release the inode
    // inode->put();

    return 0;
}

int File::read(char* buf, size_t size)
{
    if (ops.read)
        return ops.read(this, buf, size, offset);

    kprintf(WARN "read(): called but not implemented\n");
    return -1;
}

int File::write(const char* buf, size_t size)
{
    if (ops.write)
        return ops.write(this, buf, size, offset);

    kprintf(WARN "write(): called but not implemented\n");
    return -1;
}

int File::seek(size_t offset, int whence)
{
    if (ops.seek)
        return ops.seek(this, offset, whence);

    kprintf(WARN "seek(): called but not implemented\n");
    return -1;
}

File* FileTable::alloc()
{
    for (File* file = &files[0]; file < &files[FILE_TABLE_SIZE]; file++)
        if (file->refs == 0)
            return file;

    kprintf(WARN "alloc(): file table is full\n");
    return nullptr;
}

void FileTable::debug()
{
    kprintf("File table:\n{\n");

    for (int i = 0; i < FILE_TABLE_SIZE; i++)
    {
        File* file = &files[i];

        if (file->refs == 0)
            continue;

        kprintf("    %d: offset=%lu inode=%a flags=%x refs=%d ", i, file->offset, file->inode, file->flags, file->refs);

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

        kprintf("\n");
    }

    kprintf("}\n");
}