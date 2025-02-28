#include <arch/gdt.h>

extern "C" void load_gdt_desc(GDTDescriptor* desc);

alignas(0x1000) GDT gdt;
alignas(16) TSS tss;

void TSSDescriptor::set(u64 base, u32 limit, u8 _acces, u8 flags)
{
    ((GDTEntry*)this)->set(base, limit, _acces, flags);

    // base_low = base & 0xffff;
    // base_mid = (base >> 16) & 0xff;
    // base_high = (base >> 24) & 0xff;
    base_high2 = base >> 32;

    // limit_low = limit & 0xffff;
    // flags_and_limit_high = ((limit >> 16) & 0x0f) | (flags << 4);

    // access = _acces;
}

void GDTEntry::set(u32 base, u32 limit, u8 _access, u8 flags)
{
    base_low = base & 0xffff;
    base_mid = (base >> 16) & 0xff;
    base_high = (base >> 24) & 0xff;

    limit_low = limit & 0xffff;
    flags_and_limit_high = ((limit >> 16) & 0x0f) | (flags << 4);

    access = _access;
}

void GDT::init()
{
    kprintf(INFO "Initializing GDT...\n");

    null.set(0, 0, 0, 0);

    kernel_code.set(0, 0, 0b10011010, 0b0010);
    kernel_data.set(0, 0, 0b10010010, 0b0000);

    // these two are reversed compared to the kernel
    // so data before code because of retarded sysret
    user_data.set(0, 0, 0b11110010, 0b0000);
    user_code.set(0, 0, 0b11111010, 0b0010);

    tss_desc.set((u64)&tss, sizeof(TSS) - 1, 0b10001001, 0);

    GDTDescriptor desc;
    desc.size = sizeof(GDT) - 1;
    desc.offset = (u64)this;

    load_gdt_desc(&desc);

    memset(&tss, 0, sizeof(TSS));
    tss.iomap_base = sizeof(TSS);

    asm volatile("ltr %0" : : "r"((u16)0x28));

    // IA32_KERNEL_GS_BASE
    wrmsr(0xC0000102, (u64)&tss);

    asm volatile("swapgs");
}