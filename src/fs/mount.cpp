#include <fs/mount.h>
#include <mem/heap.h>

Mount mounts[MOUNT_TABLE_SIZE];
Mount* root_mount;

result_ptr<Mount> alloc_mount()
{
    for (Mount* mnt = &mounts[0]; mnt < &mounts[MOUNT_TABLE_SIZE]; mnt++)
        if (!mnt->sb)
            return mnt;

    return -ERR_MNT_TABLE_FULL;
}

Mount* Mount::find(Inode* inode)
{
    for (Mount* mnt = &mounts[0]; mnt < &mounts[MOUNT_TABLE_SIZE]; mnt++)
        if (mnt->sb && mnt->mp.inode == inode)
            return mnt;

    return nullptr;
}

Mount* Mount::find(Superblock* sb)
{
    for (Mount* mnt = &mounts[0]; mnt < &mounts[MOUNT_TABLE_SIZE]; mnt++)
        if (mnt->sb && mnt->sb == sb)
            return mnt;

    return nullptr;
}

int Mount::fill_mount(Mount* mnt, Device* dev, Filesystem* fs)
{
    if (fs->requires_device() && !dev)
        return -ERR_NO_DEV;

    // maybe check also warn if fs doesnt require device but dev is provided
    // more care if the device is already in a mount

    auto sb = fs->create_sb(fs, dev);

    if (!sb)
        return sb.error();

    fs->num_sb++;
    mnt->sb = sb.ptr;

    return 0;
}

result_ptr<Inode> get_target(const char* target)
{
    auto inode = Inode::get(target);

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

    return inode;
}

result_ptr<Mount> Mount::mount(const char* target, Device* dev, Filesystem* fs)
{
    // this gives a new reference so we need to ->put on error
    auto inode = get_target(target);

    if (!inode)
        return inode.error();

    auto mnt = alloc_mount();

    if (!mnt)
    {
        inode->put();
        return mnt.error();
    }

    int err = fill_mount(mnt.ptr, dev, fs);

    if (err)
    {
        inode->put();
        return err;
    }

    mnt->mp.path = strdup(target);
    mnt->mp.inode = inode.ptr;
    mnt->parent = Mount::find(inode->sb);
    mnt->parent->submounts++;

    return mnt;
}

result_ptr<Mount> Mount::mount_root(Device* dev, Filesystem* fs)
{
    if (root_mount)
        return -ERR_MNT_EXISTS;

    auto mnt = alloc_mount();

    if (!mnt)
        return mnt.error();

    int err = fill_mount(mnt.ptr, dev, fs);

    if (err)
        return err;

    mnt->mp.path = strdup("/");
    mnt->mp.inode = nullptr;
    mnt->parent = nullptr;

    root_mount = mnt.ptr;

    return mnt;
}

int Mount::unmount()
{
    if (this == root_mount || submounts || inode_table.get_sb_refs(sb) > 1)
        return -ERR_MNT_BUSY;

    // maybe should sync here

    sb->destroy();
    sb = nullptr;

    kfree(mp.path);

    mp.inode->put();
    mp.inode = nullptr;

    parent->submounts--;
    parent = nullptr;

    return 0;
}

void debug_mounts()
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