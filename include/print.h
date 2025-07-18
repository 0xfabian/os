#pragma once

#include <cstdarg>
#include <arch/cpu.h>
#include <fbterm.h>

#define INFO    "[\e[92mINFO\e[m]    "
#define WARN    "[\e[93mWARN\e[m]    "
#define PANIC   "[\e[91mPANIC\e[m]   "

void kprintf(const char* fmt, ...);
usize snprintf(char* buf, usize n, const char* fmt, ...);
void hexdump(void* data, usize len);

#define panic(fmt, args...)                                                 \
    do {                                                                    \
        kprintf("\a" PANIC "%s:%d: " fmt "\n", __FILE__, __LINE__, ##args); \
        idle();                                                             \
    } while (0)
