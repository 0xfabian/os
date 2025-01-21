#pragma once

#include <cstdarg>
#include <fbterm.h>

#define FATAL   "[\e[91mFATAL\e[m] "
#define INFO    "[\e[92mINFO\e[m] "
#define WARN    "[\e[93mWARN\e[m] "

void kprintf(const char* fmt, ...);
void hexdump(void* data, size_t len);