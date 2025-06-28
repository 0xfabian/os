#include <arch/cpu.h>

#define MSR_EFER    0xC0000080
#define MSR_STAR    0xC0000081
#define MSR_LSTAR   0xC0000082
#define MSR_FMASK   0xC0000084

void setup_syscall(u64 syscall_handler_address)
{
    // enable syscall/sysret instructions
    u64 efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);

    // syscall will change cs to LSTAR[47:32] and ss to LSTAR[47:32] + 8
    // so we set LSTAR[47:32] to KERNEL_CS

    // because sysret support both 64 and 32 compatibility mode
    // this is more difficult then it needs to be
    //
    // with 64 bit sysret, cs will be set to LSTAR[63:48] + 16 and ss to LSTAR[63:48] + 8
    //
    // so if we set LSTAR[63:48] to (USER_DS - 8)
    // cs will be (USER_DS - 8) + 16 = USER_DS + 8 = USER_CS
    // ss will be (USER_DS - 8) + 8 = USER_DS

    wrmsr(MSR_STAR, (u64)(USER_DS - 8) << 48 | (u64)KERNEL_CS << 32);

    wrmsr(MSR_LSTAR, syscall_handler_address);

    // on syscall, rflags is saved in r11
    // and clears the bits marked in FMASK
    // because we dont want interrupts while handling the syscall
    // we set the IF bit

    wrmsr(MSR_FMASK, 1 << 9);
}

bool check_pat_support()
{
    u32 eax, ecx, edx, ebx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    return edx & (1 << 16);
}

bool check_apic_support()
{
    u32 eax, ecx, edx, ebx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    return edx & (1 << 9);
}

bool check_x2apic_support()
{
    u32 eax, ecx, edx, ebx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    return ecx & (1 << 21);
}

void enable_sse()
{
    u64 cr0 = read_cr0();

    cr0 &= ~(1 << 2);   // EM
    cr0 |= (1 << 1);    // MP

    write_cr0(cr0);

    u64 cr4 = read_cr4();

    cr4 |= (1 << 9);    // OSFXSR
    cr4 |= (1 << 10);   // OSXMMEXCPT

    write_cr4(cr4);
}
