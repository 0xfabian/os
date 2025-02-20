#include <fs/fstype.h>

FilesystemTable fs_table;

Filesystem* Filesystem::find(const char* name)
{
    return fs_table.find(name);
}

bool Filesystem::requires_device()
{
    return flags & FS_REQUIRES_DEV;
}

void Filesystem::register_self()
{
    fs_table.add(this);
}

void Filesystem::unregister()
{
    if (num_sb)
    {
        kprintf(WARN "unregister(): filesystem still has active superblocks\n");
        return;
    }

    fs_table.remove(this);
}

void FilesystemTable::add(Filesystem* fs)
{
    for (int i = 0; i < FS_TABLE_SIZE; i++)
    {
        if (!filesystems[i])
        {
            filesystems[i] = fs;
            return;
        }
    }

    kprintf(WARN "register_fs(): filesystem table full\n");
}

void  FilesystemTable::remove(Filesystem* fs)
{
    for (int i = 0; i < FS_TABLE_SIZE; i++)
    {
        if (filesystems[i] == fs)
        {
            filesystems[i] = nullptr;
            return;
        }
    }

    kprintf(WARN "unregister_fs(): filesystem not found\n");
}

Filesystem* FilesystemTable::find(const char* name)
{
    for (int i = 0; i < FS_TABLE_SIZE; i++)
        if (filesystems[i] && !strcmp(filesystems[i]->name, name))
            return filesystems[i];

    return nullptr;
}

void FilesystemTable::debug()
{
    kprintf("Filesystem table:\n{\n");

    for (int i = 0; i < FS_TABLE_SIZE; i++)
        if (filesystems[i])
            kprintf("    %s\n", filesystems[i]->name);

    kprintf("}\n");
}