#include <print.h>
#include <memory/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>

extern "C" void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
        idle();

    fbterm.init();

    pfa.init();

    fbterm.enable_backbuffer();

    gdt.init();

    idt.init();

    heap.init(2000);

    idle();
}