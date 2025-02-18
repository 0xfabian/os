#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>

#include <fs/file.h>

int fbterm_write(File* file, const char* buf, size_t size, size_t offset)
{
    fbterm.write(buf, size);
    return size;
}

struct FileOps fbterm_ops = {
    .write = fbterm_write,
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

    File* file = File::open("/dev/fbterm", 0);

    if (!file)
        idle();

    file->ops = fbterm_ops;

    file_table.debug();

    file->write("hello\n", 6);
    file->write("world\n", 6);
    file->close();

    idle();
}