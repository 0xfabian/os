#pragma once

// #include <fs/inode.h>
#include <cstdint>
#include <cstddef>

struct File;

struct FileOps
{
    int (*open) (File* file);
    int (*close) (File* file);
    int (*read) (File* file, char* buf, size_t size, size_t offset);
    int (*write) (File* file, const char* buf, size_t size, size_t offset);
    int (*seek) (File* file, size_t offset, int whence);
};

struct File
{
    size_t offset;
    // Inode* inode;
    uint32_t flags;
    int refs;

    FileOps ops;

    static File* open(const char* path, uint32_t flags)
    {
        // Result<Inode*> inode = namei(path);

        // if (!inode)
        //     return inode.err;

        // File* file = file_table.alloc();

        // if (!file)
        //     return ENOMEM;

        // file->offset = 0;
        // file->flags = flags;
        // file->refs = 1;
        // file->inode = inode.value;
        // file->ops = inode.value->fops;

        // file->open();

        // return file;

        return nullptr;
    }

    int open()
    {
        // if (inode->is_device() && ops.open)
        //     return ops.open(this);

        return 0;
    }

    int close()
    {
        // more then one reference just decrement and return
        if (refs > 1)
        {
            refs--;
            return 0;
        }

        // last reference, we need to close the file
        refs = 0;

        // wake up process if the file is a pipe
        // if (is_pipe())
        //     wakeup_process();

        // if the file is a device call the devices close function
        // if (inode->is_device() && ops.close)
        //     ops.close(this);

        // now the file is gone so we should release the inode
        // inode_table.put(inode);

        return 0;
    }

    int read(char* buf, size_t size)
    {
        if (ops.read)
            return ops.read(this, buf, size, offset);

        return -1;
    }

    int write(const char* buf, size_t size)
    {
        if (ops.write)
            return ops.write(this, buf, size, offset);

        return -1;
    }

    int seek(size_t offset, int whence)
    {
        if (ops.seek)
            return ops.seek(this, offset, whence);

        return -1;
    }
};

#define FILE_TABLE_SIZE 256

struct FileTable
{
    File files[FILE_TABLE_SIZE];

    void init()
    {
        for (int i = 0; i < FILE_TABLE_SIZE; i++)
            files[i].refs = 0;
    }

    File* alloc()
    {
        for (File* file = &files[0]; file < &files[FILE_TABLE_SIZE]; file++)
            if (file->refs == 0)
                return file;

        return nullptr;
    }
};