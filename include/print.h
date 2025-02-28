#pragma once

#include <cstdarg>
#include <arch/cpu.h>
#include <fbterm.h>

#define INFO    "[\e[92mINFO\e[m]    "
#define WARN    "[\e[93mWARN\e[m]    "
#define PANIC   "[\e[91mPANIC\e[m]   "

void kprintf(const char* fmt, ...);
void ikprintf(const char* fmt, ...);
void hexdump(void* data, size_t len);
void panic(const char* msg);