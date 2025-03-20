#pragma once

#include <utils.h>

usize strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, usize n);
char* strcpy(char* dest, const char* src);
char* strdup(const char* str);
char* strcat(char* dest, const char* src);

char* path_read_next(const char*& ptr);
char* path_normalize(const char* path);
const char* dirname(const char* path);
const char* basename(const char* path);