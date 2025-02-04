#pragma once

#include <cstdarg>
#include <arch/cpu.h>
#include <fbterm.h>

#define OK      "\e[92mOK\e[m\n"
#define PANIC   "[\e[91mPANIC\e[m] "
#define WARN    "[\e[93mWARN\e[m] "

void kprintf(const char* fmt, ...);
void hexdump(void* data, size_t len);
void panic(const char* msg);