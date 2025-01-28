#include <arch/gdt.h>

extern "C" void load_gdt_desc(GDTDescriptor* desc);

alignas(0x1000) GDT gdt;

void GDTEntry::set(uint32_t base, uint32_t limit, uint8_t _access, uint8_t flags)
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
    null.set(0, 0, 0, 0);
    kernel_code.set(0, 0, 0b10011010, 0b0010);
    kernel_data.set(0, 0, 0b10010010, 0b0000);
    user_code.set(0, 0, 0b11111010, 0b0010);
    user_data.set(0, 0, 0b11110010, 0b0000);

    GDTDescriptor desc;
    desc.size = sizeof(GDT) - 1;
    desc.offset = (uint64_t)this;

    load_gdt_desc(&desc);
}