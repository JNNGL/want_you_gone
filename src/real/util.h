#pragma once

#include <stdint.h>

void putc16(char ch);
void fputs16(const char *str);
void puts16(const char* str);
void print64h16(uint64_t x);
void die16(const char* str);
