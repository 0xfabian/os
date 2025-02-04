#include <print.h>
#include <mem/heap.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <ds/list.h>

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

    List<int> list;

    list.add(21);
    list.add(23);

    for (auto& a : list)
        kprintf("%d\n", a);

    idle();
}