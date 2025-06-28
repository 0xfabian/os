#include <fs/fstype.h>

Filesystem* fs_list = nullptr;
Filesystem* tail = nullptr;

// name should be unique for each filesystem
Filesystem* Filesystem::find(const char* name)
{
    for (Filesystem* fs = fs_list; fs; fs = fs->next)
        if (strcmp(fs->name, name) == 0)
            return fs;

    return nullptr;
}

int Filesystem::register_self()
{
    Filesystem* fs = find(name);

    if (fs)
        return (fs == this) ? 0 : -ERR_FS_EXISTS;

    registered = true;
    num_sb = 0;
    next = nullptr;

    if (fs_list)
    {
        tail->next = this;
        tail = this;
    }
    else
    {
        fs_list = this;
        tail = this;
    }

    return 0;
}

int Filesystem::unregister()
{
    if (!registered)
        return 0;

    if (num_sb)
        return -ERR_FS_BUSY;

    Filesystem* prev = nullptr;
    Filesystem* fs = fs_list;

    while (fs)
    {
        if (fs == this)
        {
            if (prev)
                prev->next = next;
            else
                fs_list = next;

            if (tail == this)
                tail = prev;

            registered = false;

            return 0;
        }

        prev = fs;
        fs = fs->next;
    }

    return -ERR_NOT_FOUND;
}

bool Filesystem::requires_device()
{
    return flags & FS_REQUIRES_DEV;
}

void debug_filesystems()
{
    kprintf("Filesystems:\n{\n");

    for (Filesystem* fs = fs_list; fs; fs = fs->next)
        kprintf("    %s\n", fs->name);

    kprintf("}\n");
}
