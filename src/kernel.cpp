#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>

#include <fs/mount.h>
#include <fs/ramfs/ramfs.h>
#include <task.h>

void ls(const char* path)
{
    kprintf("> ls %s\n", path);

    auto dir = File::open(path, 0);

    if (!dir)
    {
        kprintf("Failed to open `%s`: %d\n", path, dir.error());
        return;
    }

    char buf[256];
    while (dir->iterate(buf, sizeof(buf)) > 0)
        kprintf("%s  ", buf);

    kprintf("\n");

    dir->close();
}

extern "C" void task1()
{
    int n = 0;

    while (true)
        kprintf("\e[91m%d\e[m\n", n++);
}

extern "C" void task2()
{
    int n = 0;

    while (true)
        kprintf("\e[92m%d\e[m\n", n++);
}

extern "C" void task3()
{
    int n = 0;

    while (true)
        kprintf("\e[94m%d\e[m\n", n++);
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

    // ramfs.register_self();

    // Mount::mount_root(nullptr, &ramfs);

    // ls("/");

    // auto inode = Inode::get("/");

    // if (inode)
    // {
    //     inode->create("a.txt");
    //     inode->create("sh");
    //     inode->mkdir("bin");
    //     inode->put();
    // }

    // ls("/");
    // ls("/bin");

    // inode = Inode::get("/");

    // if (inode)
    // {
    //     inode->unlink("a.txt");
    //     inode->put();
    // }

    // ls("/");

    // inode_table.debug();
    // heap.debug();

    Task* t0 = Task::dummy();
    Task* t1 = Task::from(task1);
    Task* t2 = Task::from(task2);
    Task* t3 = Task::from(task3);

    t0->ready();
    t1->ready();
    t2->ready();
    t3->ready();

    Task* t = task_list;

    do
    {
        kprintf("Task %lu: %p -> %p\n", t->tid, t, t->next);
        t = t->next;
    } while (t != task_list);

    running = task_list;

    pic::set_irq(0, false);
    sti();

    // at this kmain continues as t0

    // while (true)
    //     kprintf("kernel task\n");

    kprintf("Task %lu: begin idle\n", running->tid);
    idle();
}