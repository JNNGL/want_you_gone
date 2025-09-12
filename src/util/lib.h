#pragma once

#include <stddef.h>

size_t strlen(const char* str);
int strcmp(const char* str1, const char* str2);

void* memset(void* ptr, int value, size_t size);
void* memcpy(void* restrict dst, const void* restrict src, size_t n);
void* memmove(void* restrict dst, int value, size_t n);

void* malloc(size_t n);
void* calloc(size_t n);
void* malloca(size_t n, size_t align);
void* calloca(size_t n, size_t align);