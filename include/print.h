#pragma once

#include <cstdarg>
#include <fbterm.h>

#define FATAL   "[\e[91mFATAL\e[0m] "
#define INFO    "[\e[92mINFO\e[0m] "
#define WARN    "[\e[93mWARN\e[0m] "

void kprintf(const char* fmt, ...);