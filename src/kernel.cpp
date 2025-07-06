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

void load_exec(const char* path, const u8* data, usize size)
{
    auto file = File::open(path, O_WRONLY | O_CREAT, IP_RWX);
    file->write((const char*)data, size);
    file->close();
}

void populate_root()
{
    auto inode = Inode::get("/");
    inode->mkdir("dev");
    inode->mkdir("bin");
    inode->mkdir("mnt");
    inode->put();

    inode = Inode::get("/dev");
    inode->mknod("fb", IT_CDEV | IP_RW, 0x80);
    inode->mknod("fbterm", IT_CDEV | IP_RW, 0x81);
    inode->mknod("null", IT_CDEV | IP_RW, 0x82);
    inode->mknod("zero", IT_CDEV | IP_RW, 0x83);
    inode->mknod("random", IT_CDEV | IP_RW, 0x84);
    inode->mknod("serial", IT_CDEV | IP_RW, 0x85);
    inode->put();

    load_exec("/bin/sh", sh_code, sizeof(sh_code));
    load_exec("/bin/echo", echo_code, sizeof(echo_code));
    load_exec("/bin/ls", ls_code, sizeof(ls_code));
    load_exec("/bin/clear", clear_code, sizeof(clear_code));
    load_exec("/bin/ed", ed_code, sizeof(ed_code));
    load_exec("/bin/cow", cow_code, sizeof(cow_code));

    auto file = File::open("/a", O_WRONLY | O_CREAT, IP_RW);
    file->write("/mnt/lib/sys.o -I/mnt/include -nostdlib -static\n", 48);
    file->close();
}

extern "C" void kmain(void)
{
    if (!LIMINE_BASE_REVISION_SUPPORTED)
        idle();

    enable_sse();

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

    populate_root();

    if (!ata_bdev.init() || !Mount::mount("/mnt", &ata_bdev, &ext2fs))
        kprintf(WARN "Failed to mount disk\n");

    sched_init();

    kbd_task = Task::from(keyboard_task, "kbd");
    kbd_task->ready();

    Task* sh = Task::from("/mnt/bin/sh");

    if (!sh)
        sh = Task::from("/bin/sh");

    if (!sh)
        panic("Failed to start a shell");

    sh->ready();

    running->exit(0);
}
