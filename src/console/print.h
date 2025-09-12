#pragma once

#include <stdint.h>

void init_console(void* font);
void load_font(void* ptr);
void putc(char ch);
void kputs(const char* str);
void puts(const char* str);

void clear_console();
void console_set_x(uint32_t x);
void console_set_y(uint32_t y);