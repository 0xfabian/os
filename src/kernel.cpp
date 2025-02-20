#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>

#include <fs/mount.h>
#include <fs/ramfs/ramfs.h>

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

    heap.debug();

    ramfs.register_self();

    Mount::mount_root(nullptr, &ramfs);

    mount_table.debug();

    inode_table.debug();

    file_table.debug();

    kprintf("File::open(\"/hello.txt\", 0)\n");
    File* file = File::open("/hello.txt", 0);

    inode_table.debug();
    file_table.debug();

    char buf[256];

    kprintf("File::read(buf, 100)\n");
    size_t bytes = file->read(buf, 100);
    kprintf("returned %d bytes\n", bytes);

    for (size_t i = 0; i < bytes; i++)
        kprintf("%c", buf[i]);

    kprintf("file->close()\n");
    file->close();

    inode_table.debug();
    file_table.debug();

    heap.debug();

    kprintf("root_mount->unmount()\n");
    root_mount->unmount();

    heap.debug();

    ramfs.unregister();

    heap.debug();

    idle();
}