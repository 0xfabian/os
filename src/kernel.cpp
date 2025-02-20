#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>

#include <fs/mount.h>

Superblock* ext2_mount(Device* dev)
{
    return (Superblock*)1;
}

Filesystem ext2 =
{
    .name = "ext2",
    .mount = ext2_mount
};

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

    ext2.register_self();

    fs_table.debug();

    Mount::mount_root(nullptr, &ext2);

    mount_table.debug();

    idle();
}