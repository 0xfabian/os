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

    ramfs.register_self();

    Mount::mount_root(nullptr, &ramfs);

    File* file = File::open("/", 0);

    if (file)
    {
        char buf[256];

        while (file->iterate(buf, 256) > 0)
            kprintf("%s\n", buf);

        file->seek(0, SEEK_SET);

        while (file->iterate(buf, 256) > 0)
            kprintf("%s\n", buf);

        file->close();
    }

    idle();
}