#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <fs/mount.h>
#include <fs/ramfs/ramfs.h>
#include <fs/ext2/ext2.h>
#include <task.h>
#include <kbd.h>
#include <drivers/bdev.h>

#include <bin.h>

extern "C" void kmain(void)
{
    if (!LIMINE_BASE_REVISION_SUPPORTED)
        idle();

    fbterm.init();

    pmm.init();

    vmm.init();

    fbterm.enable_backbuffer();

    gdt.init();

    idt.init();

    heap.init(2000);

    ramfs.register_self();
    ext2fs.register_self();

    Mount::mount_root(nullptr, &ramfs);

    auto inode = Inode::get("/");
    inode->mkdir("dev");
    inode->mkdir("mnt");
    inode->create("sh");
    inode->create("echo");
    inode->create("ls");
    inode->create("clear");
    inode->put();
    inode = Inode::get("/dev");
    inode->mknod("fbterm", 0x81);
    inode->put();

    auto file = File::open("/sh", 0);
    file->write((const char*)sh_code, sizeof(sh_code));
    file->close();

    file = File::open("/echo", 0);
    file->write((const char*)echo_code, sizeof(echo_code));
    file->close();

    file = File::open("/ls", 0);
    file->write((const char*)ls_code, sizeof(ls_code));
    file->close();

    file = File::open("/clear", 0);
    file->write((const char*)clear_code, sizeof(clear_code));
    file->close();

    auto mnt = Mount::mount("/mnt", &ata_bdev, &ext2fs);

    if (!mnt)
        kprintf(WARN "Failed to mount /mnt\n");

    sched_init();

    Task* kbd = Task::from(keyboard_task);
    Task* sh = Task::from("/sh");

    kbd->ready();
    sh->ready();

    idle();
}