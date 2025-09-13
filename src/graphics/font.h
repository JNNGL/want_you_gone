#pragma once

#include <stdint.h>

void init_font(void* data);
void draw_char(char c);
void set_text_x(uint32_t x);
void set_text_y(uint32_t y);
void set_line_height(int a, int b);
void reset_text_dirty_rect();
uint32_t get_font_width();

void clear_text();
void tick_clear();

void set_cursor_state(uint8_t state);
void toggle_cursor_state();