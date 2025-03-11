#pragma once

#include <utils.h>

usize strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, usize n);
char* strcpy(char* dest, const char* src);
char* strdup(const char* str);