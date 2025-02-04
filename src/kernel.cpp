#include <requests.h>
#include <memory/heap.h>
#include <print.h>
#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/idt.h>

extern "C" void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
        idle();

    limine_framebuffer_response* framebuffer_response = framebuffer_request.response;

    if (!framebuffer_response || framebuffer_response->framebuffer_count == 0)
        idle();

    default_fb.init(framebuffer_response->framebuffers[0]);

    limine_memmap_response* memmap_response = memmap_request.response;

    if (!memmap_response || !pfa.init(memmap_response))
        idle();

    size_t count = PAGE_COUNT(default_fb.width * default_fb.height * sizeof(uint32_t));
    backbuffer = (uint32_t*)((uint64_t)pfa.alloc_pages(count) | 0xffff800000000000);

    if (!backbuffer)
        idle();

    fbterm.init();

    kprintf(INFO "%dx%d fbterm initialized\n", fbterm.width, fbterm.height);
    kprintf(INFO "pfa initialized %d/%d\n", pfa.used_pages, pfa.total_pages);
    kprintf(INFO "backbuffer allocated at %a\n", backbuffer);

    kprintf(INFO "initializing gdt\n");

    gdt.init();

    kprintf(INFO "initializing idt\n");

    idt.init();

    kprintf(INFO "initializing heap\n");

    heap.init(200);

    if (!heap.start)
        idle();

    int* a = (int*)heap.alloc(sizeof(int));
    int* b = (int*)heap.alloc(100 * sizeof(int));
    MemoryRegion* mr = (MemoryRegion*)heap.alloc(sizeof(MemoryRegion));

    kprintf("heap_header: %lu\n", sizeof(Heap::Header));

    Heap::Header* ptr = heap.start;

    while (ptr)
    {
        kprintf("%a: prev: %a next: %a size: %lu free: %f\n", ptr, ptr->prev, ptr->next, ptr->size(), ptr->is_free());
        ptr = ptr->next;
    }

    heap.free(a);
    heap.free(b);

    kprintf("after\n");

    ptr = heap.start;

    while (ptr)
    {
        kprintf("%a: prev: %a next: %a size: %lu free: %f\n", ptr, ptr->prev, ptr->next, ptr->size(), ptr->is_free());
        ptr = ptr->next;
    }

    heap.free(mr);

    idle();
}