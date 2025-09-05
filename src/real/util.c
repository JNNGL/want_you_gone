__asm__(".code16gcc");

#include "util.h"

void putc16(char ch) {
    asm volatile("int $0x10" : : "a"(0x0E00 | ch));
}

void fputs16(const char *str) {
    while (*str != 0) {
        putc16(*str++);
    }
}

void puts16(const char* str) {
    fputs16(str);
    fputs16("\n\r");
}

void print64h16(uint64_t x) {
    for (uint64_t i = 0; i < 16; i++) {
        uint8_t d = (x & ((0xFull << 60) >> (i * 4))) >> (60 - i * 4);
        putc16("0123456789ABCDEF"[d & 0xF]);
    }
}

void die16(const char* str) {
    puts16(str);
    while (1) {
        asm("hlt");
    }
}