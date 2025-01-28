#include <requests.h>
#include <memory/pfalloc.h>
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
    backbuffer = (uint32_t*)default_fb.addr;
    fbterm.init();

    kprintf(INFO "%dx%d fbterm initialized\n", fbterm.width, fbterm.height);
    kprintf(WARN "no backbuffer\n");

    limine_memmap_response* memmap_response = memmap_request.response;

    if (!memmap_response || !pfa.init(memmap_response))
        idle();

    kprintf(INFO "pfa initialized %d/%d\n", pfa.used_pages, pfa.total_pages);

    size_t count = PAGE_COUNT(default_fb.width * default_fb.height * sizeof(uint32_t));
    backbuffer = (uint32_t*)((uint64_t)pfa.alloc_pages(count) | 0xffff800000000000);
    memcpy(backbuffer, (void*)default_fb.addr, count * PAGE_SIZE);

    kprintf(INFO "backbuffer allocated at %a\n", backbuffer);

    gdt.init();

    kprintf(INFO "gdt initialized\n");

    idt.init();

    kprintf(INFO "idt initialized\n");

    idle();
}