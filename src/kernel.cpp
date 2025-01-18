#include <limine.h>
#include <print.h>

#define KB(x) ((x) / 1024)
#define MB(x) ((x) / 1048576)
#define GB(x) ((x) / 1073741824) 

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .flags = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void idle()
{
    while (true)
        asm("hlt");
}

extern "C" void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
        idle();

    limine_framebuffer_response* framebuffer_response = framebuffer_request.response;
    limine_smp_response* smp_response = smp_request.response;
    limine_memmap_response* memmap_response = memmap_request.response;

    if (framebuffer_response && framebuffer_response->framebuffer_count > 0)
    {
        default_fb.init(framebuffer_response->framebuffers[0]);
        fbterm.init();

        kprintf(INFO "default framebuffer initialized\n");
        kprintf(INFO "fbterm initialized %dx%d\n", fbterm.width, fbterm.height);
    }
    else
        idle();

    if (smp_response)
        kprintf(INFO "cpu count: %d\n", smp_response->cpu_count);
    else
    {
        kprintf(WARN "no smp response\n");
        idle();
    }

    if (memmap_response)
    {
        uint64_t total_memory = 0;
        uint64_t usable_memory = 0;

        kprintf(INFO "memory map response\n");

        const char* type_str[] = {
            "\e[92musable\e[m",
            "reserved",
            "ACPI reclaimable",
            "ACPI NVS",
            "bad memory",
            "\e[92mbootloader reclaimable\e[m",
            "executable and modules",
            "framebuffer"
        };


        for (uint64_t i = 0; i < memmap_response->entry_count; i++)
        {
            limine_memmap_entry* entry = memmap_response->entries[i];
            total_memory += entry->length;

            if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
                usable_memory += entry->length;

            kprintf("[%lx, %lx] %s\n", entry->base, entry->base + entry->length, type_str[entry->type]);
        }

        kprintf("total: %ld MB   usable: %ld MB\n", MB(total_memory), MB(usable_memory));
    }
    else
    {
        kprintf(FATAL "no memory map response\n");
        idle();
    }

    idle();
}