#include <fs/fstype.h>

FilesystemType* head = nullptr;
FilesystemType* tail = nullptr;

void register_fs(FilesystemType* fs)
{
    fs->next = nullptr;
    fs->active_mounts = 0;

    if (!head)
    {
        head = fs;
        tail = head;
    }
    else
    {
        tail->next = fs;
        tail = fs;
    }
}

int unregister_fs(FilesystemType* fs)
{
    FilesystemType* prev = nullptr;
    FilesystemType* current = head;

    while (current)
    {
        if (current == fs)
        {
            if (fs->active_mounts)
            {
                kprintf(WARN "unregister_fs(): filesystem has active mounts\n");
                return -1;
            }

            if (prev)
                prev->next = current->next;
            else
                head = current->next;

            if (current == tail)
                tail = prev;

            return 0;
        }

        prev = current;
        current = current->next;
    }

    kprintf(WARN "unregister_fs(): filesystem not found\n");
    return -1;
}

FilesystemType* find_fs(const char* name)
{
    FilesystemType* fs = head;

    while (fs)
    {
        if (strcmp(fs->name, name) == 0)
            return fs;

        fs = fs->next;
    }

    return nullptr;
}