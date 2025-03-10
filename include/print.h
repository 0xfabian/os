#pragma once

#include <cstdarg>
#include <arch/cpu.h>
#include <fbterm.h>

#define INFO    "[\e[96mINFO\e[m]    "
#define WARN    "[\e[93mWARN\e[m]    "
#define PANIC   "[\e[91mPANIC\e[m]   "

void kprintf(const char* fmt, ...);
void ikprintf(const char* fmt, ...);
void hexdump(void* data, usize len);
void panic(const char* msg);