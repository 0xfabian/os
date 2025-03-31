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

void enable_sse()
{
    uint64_t cr0, cr4;

    // Read CR0
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    // Clear EM (bit 2, x87 emulation must be disabled)
    cr0 &= ~(1 << 2);
    // Set MP (bit 1, must be set when EM is 0)
    cr0 |= (1 << 1);
    // Write back CR0
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));

    // Read CR4
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));
    // Set OSFXSR (bit 9, enable SSE)
    cr4 |= (1 << 9);
    // Set OSXMMEXCPT (bit 10, enable SSE exceptions)
    cr4 |= (1 << 10);
    // Write back CR4
    asm volatile ("mov %0, %%cr4" :: "r"(cr4));
}

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
    inode->mkdir("bin");
    inode->mkdir("mnt");
    inode->put();

    inode = Inode::get("/dev");
    inode->mknod("fbterm", 0x81);
    inode->put();

    inode = Inode::get("/bin");
    inode->create("sh");
    inode->create("echo");
    inode->create("ls");
    inode->create("clear");
    inode->create("ed");
    inode->put();

    auto file = File::open("/bin/sh", 0);
    file->write((const char*)sh_code, sizeof(sh_code));
    file->close();

    file = File::open("/bin/echo", 0);
    file->write((const char*)echo_code, sizeof(echo_code));
    file->close();

    file = File::open("/bin/ls", 0);
    file->write((const char*)ls_code, sizeof(ls_code));
    file->close();

    file = File::open("/bin/clear", 0);
    file->write((const char*)clear_code, sizeof(clear_code));
    file->close();

    file->File::open("/bin/ed", 0);
    file->write((const char*)ed_data, sizeof(ed_data));
    file->close();

    auto mnt = Mount::mount("/mnt", &ata_bdev, &ext2fs);

    if (!mnt)
        kprintf(WARN "Failed to mount /mnt\n");

    enable_sse();

    sched_init();

    kbd_task = Task::from(keyboard_task);
    kbd_task->state = TASK_SLEEPING;

    Task* sh = Task::from(mnt ? "/mnt/bin/sh" : "/bin/sh");

    sh->ready();

    running->exit(0);
}