#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>

#include <fs/mount.h>
#include <fs/ramfs/ramfs.h>

void mount(const char* target, const char* dev, const char* fs_name)
{
    Filesystem* fs = Filesystem::find(fs_name);

    if (!fs)
    {
        kprintf("Failed to find filesystem %s\n", fs_name);
        return;
    }

    Device* bdev = nullptr;

    if (dev[0] == '\0')
        bdev = nullptr;
    else if (root_mount)
        bdev = (Device*)Inode::get(dev).or_nullptr();

    if (target[0] == '/' && target[1] == '\0')
        Mount::mount_root(bdev, fs);
    else
        Mount::mount(target, bdev, fs);;
}

void umount(const char* target)
{
    auto inode = Inode::get(target);

    auto mnt = Mount::find(inode->sb);

    inode->put();

    mnt->unmount();
}

void list(const char* path)
{
    auto dir = File::open(path, 0);

    if (!dir || !dir->inode->is_dir())
    {
        kprintf("Failed to open directory %s %d\n", path, dir.error());
        return;
    }

    kprintf("Listing %s:\n", path);

    char buf[256];
    while (dir->iterate(buf, sizeof(buf)) > 0)
        kprintf("    %s\n", buf);

    dir->close();
}

void mkdirat(const char* path, const char* name)
{
    auto dir = File::open(path, 0);

    if (!dir || !dir->inode->is_dir())
    {
        kprintf("Failed to open directory %s %d\n", path, dir.error());
        return;
    }

    dir->inode->mkdir(name);
    dir->close();
}

extern "C" void kmain(void)
{
    if (!LIMINE_BASE_REVISION_SUPPORTED)
        idle();

    fbterm.init();

    pfa.init();

    fbterm.enable_backbuffer();

    gdt.init();

    idt.init();

    heap.init(2000);

    ramfs.register_self();

    kprintf("mounting root...\n");
    mount("/", "", "ramfs");

    inode_table.debug();

    debug_mounts();

    kprintf("creating directories...\n");
    mkdirat("/", "dev");

    inode_table.debug();

    mkdirat("/dev", "root mount");

    inode_table.debug();

    list("/dev");

    kprintf("mounting /dev...\n");
    mount("/dev/", "/", "ramfs");

    debug_mounts();

    list("/dev");

    kprintf("creating dir inside mount...\n");
    mkdirat("/dev", "inside new mount");

    list("/dev");

    kprintf("umounting /dev...\n");
    umount("/dev");

    debug_mounts();

    list("/dev");

    inode_table.debug();

    idle();
}