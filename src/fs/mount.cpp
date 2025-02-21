#include <fs/mount.h>
#include <mem/heap.h>

MountTable mount_table;
Mount* root_mount;

Mount* Mount::find(Inode* inode)
{
    return mount_table.find(inode);
}

Mount* Mount::find(Superblock* sb)
{
    return mount_table.find(sb);
}

int Mount::fill_mount(Mount* mnt, Device* dev, Filesystem* fs)
{
    if (fs->requires_device() && !dev)
    {
        kprintf(WARN "mount_root(): filesystem requires device\n");
        return -1;
    }

    // maybe check also warn if fs doesnt require device but dev is provided
    // more care if the device is already in a mount

    Superblock* sb = fs->create_sb(fs, dev);

    if (!sb)
        return -1;

    mnt->sb = sb;

    return 0;
}

int get_target(const char* target, Inode** out)
{
    result_ptr<Inode> inode = Inode::get(target);

    if (!inode)
        return inode.error();

    if (!inode->is_dir())
    {
        inode->put();

        return -ERR_NOT_DIR;
    }

    if (Mount::find(inode.ptr))
    {
        inode->put();

        return -ERR_MNT_EXISTS;
    }

    *out = inode.ptr;

    return 0;
}

int Mount::mount(const char* target, Device* dev, Filesystem* fs)
{
    Inode* inode;

    int err = get_target(target, &inode);

    if (err)
        return err;

    Mount* mnt = mount_table.alloc();

    if (!mnt)
        return -1;

    err = fill_mount(mnt, dev, fs);

    if (err)
        return err;

    mnt->mp.path = strdup(target);
    mnt->mp.inode = inode;
    mnt->parent = Mount::find(inode->sb);
    mnt->parent->submounts++;

    return 0;
}

int Mount::mount_root(Device* dev, Filesystem* fs)
{
    if (root_mount)
    {
        kprintf(WARN "mount_root(): root mount already exists\n");
        return -1;
    }

    root_mount = mount_table.alloc();

    if (!root_mount)
        return -1;

    int err = fill_mount(root_mount, dev, fs);

    if (err)
        return err;

    root_mount->mp.path = strdup("/");
    root_mount->mp.inode = nullptr;
    root_mount->parent = nullptr;

    return 0;
}

int Mount::unmount()
{
    if (this == root_mount || submounts || inode_table.get_sb_refs(sb) > 1)
    {
        kprintf(WARN "unmount(): mount is busy\n");
        return -1;
    }

    // maybe should sync here

    sb->destroy();
    sb = nullptr;

    kfree(mp.path);

    mp.inode->put();
    mp.inode = nullptr;

    parent = nullptr;

    return 0;
}

Inode* Mount::get_root()
{
    Inode* root = sb->root;
    root->refs++;

    return root;
}

Mount* MountTable::alloc()
{
    for (Mount* mnt = &mounts[0]; mnt < &mounts[MOUNT_TABLE_SIZE]; mnt++)
        if (!mnt->sb)
            return mnt;

    kprintf(WARN "alloc(): mount table is full\n");
    return nullptr;
}

Mount* MountTable::find(Inode* inode)
{
    for (Mount* mnt = &mounts[0]; mnt < &mounts[MOUNT_TABLE_SIZE]; mnt++)
        if (mnt->sb && mnt->mp.inode == inode)
            return mnt;

    return nullptr;
}

Mount* MountTable::find(Superblock* sb)
{
    for (Mount* mnt = &mounts[0]; mnt < &mounts[MOUNT_TABLE_SIZE]; mnt++)
        if (mnt->sb && mnt->sb == sb)
            return mnt;

    return nullptr;
}

void MountTable::debug()
{
    kprintf("Mount table:\n{\n");

    for (int i = 0; i < MOUNT_TABLE_SIZE; i++)
    {
        Mount* mnt = &mounts[i];

        if (!mnt->sb)
            continue;

        kprintf("    %s (%s)", mnt->mp.path, mnt->sb->fs->name);

        if (mnt->parent)
            kprintf(" inside %s", mnt->parent->mp.path);

        kprintf("\n");
    }

    kprintf("}\n");
}