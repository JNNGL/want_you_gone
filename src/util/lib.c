#include "lib.h"

#include <real/memory.h>
#include <stdint.h>

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }

    return len;
}

int strcmp(const char* str1, const char* str2) {
    const uint8_t* a = (const uint8_t*) str1;
    const uint8_t* b = (const uint8_t*) str2;
    for (size_t i = 0;; i++) {
        if (a[i] < b[i]) {
            return -1;
        } else if (a[i] > b[i]) {
            return 1;
        } else if (a[i] == 0) {
            return 0;
        }
    }
}

void* memcpy(void* restrict dst, const void* restrict src, size_t n) {
    asm volatile("cld; rep movsb" : "+c"(n), "+S"(src), "+D"(dst) : : "memory");
    return dst;
}

void* memset(void* ptr, int value, size_t size) {
    asm volatile("cld; rep stosb" : "+c"(size), "+D"(ptr) : "a"(value) : "memory");
    return ptr;
}

void* malloc(size_t n) {
    void* ptr = free_memory_ptr;
    free_memory_ptr += n;
    return ptr;
}

void* calloc(size_t n) {
    void* ptr = malloc(n);
    memset(ptr, 0, n);
    return ptr;
}

void* malloca(size_t n, size_t align) {
    free_memory_ptr = (void*) (((uintptr_t) free_memory_ptr + align - 1) / align * align);

    void* ptr = free_memory_ptr;
    free_memory_ptr += n;
    return ptr;
}

void* calloca(size_t n, size_t align) {
    void* ptr = malloca(n, align);
    memset(ptr, 0, n);
    return ptr;
}