#pragma once

#include <utils.h>

usize strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, usize n);
char* strcpy(char* dest, const char* src);
char* strdup(const char* str);
char* strcat(char* dest, const char* src);

char* path_read_next(const char*& ptr);
char* normalize_path(const char* path);