#include "font.h"

#include <graphics/background.h>
#include <real/video.h>
#include <util/lib.h>
#include <stdint.h>

static struct {
    uint32_t image_width;
    uint32_t image_height;
    uint32_t char_width;
    uint32_t char_height;
    uint32_t line_height;
    uint8_t* data;
} font_data;

static struct {
    int x;
    int y;
    int rx;
} text_position;

static struct dirty_rect {
    int x0;
    int y0;
    int x1;
    int y1;
} dirty_rect;

static union {
    struct {
        uint32_t b : 8;
        uint32_t g : 8;
        uint32_t r : 8;
    };
    uint32_t rgb;
} color;

static uint8_t cursor_active = 0;

static struct dirty_rect clear_rect;
static uint8_t clear_flag = 0;
static uint32_t clear_y = 0;

uint32_t map_unicode_character(char c) {
    if (c >= 0x21 && c <= 0x7E) {
        return c - 0x20;
    }
    return 0;
}

void init_font(void* data) {
    font_data.image_width = ((uint32_t*) data)[0];
    font_data.image_height = ((uint32_t*) data)[1];
    font_data.char_width = font_data.image_width / 16;
    font_data.char_height = font_data.image_height / 6;
    font_data.line_height = font_data.char_height * 5 / 4;
    font_data.data = data + 8;

    dirty_rect.x0 = 0;
    dirty_rect.x1 = 0;
    dirty_rect.y0 = 0;
    dirty_rect.y1 = 0;

    text_position.x = 0;
    text_position.y = 0;
    text_position.rx = 0;

    color.rgb = 0x806439;
}

__attribute__((optimize("O3")))
void draw_cursor() {
    uint32_t y_pos = text_position.y + font_data.char_height * 3 / 4;
    uint32_t* base_ptr = ((uint32_t*) (linear_framebuffer + y_pos * vbe_mode_info.pitch)) + text_position.x;
    uint32_t* background_ptr = background_buffer + y_pos * vbe_mode_info.width + text_position.x;
    for (uint32_t y = 0; y < 3; y++) {
        for (uint32_t x = font_data.char_width / 10; x < font_data.char_width * 9 / 10; x++) {
            if (cursor_active & 1) {
                base_ptr[x] = 0xFF000000 | color.rgb;
            } else {
                base_ptr[x] = background_ptr[x];
            }
        }
        base_ptr = (uint32_t*) (((void*) base_ptr) + vbe_mode_info.pitch);
        background_ptr += vbe_mode_info.width;
    }
}

__attribute__((optimize("O3")))
void draw_char(char c) {
    if (c == '\n') {
        if (cursor_active) {
            cursor_active <<= 1;
            draw_cursor();
        }

        text_position.y += font_data.line_height;
        text_position.x = text_position.rx;

        if (cursor_active >>= 1) {
            draw_cursor();
        }

        return;
    }

    if (text_position.x < dirty_rect.x0) {
        dirty_rect.x0 = text_position.x;
    }
    if (text_position.y < dirty_rect.y0) {
        dirty_rect.y0 = text_position.y;
    }
    if (text_position.x + font_data.char_width > dirty_rect.x1) {
        dirty_rect.x1 = text_position.x + font_data.char_width;
    }
    if (text_position.y + font_data.char_height > dirty_rect.y1) {
        dirty_rect.y1 = text_position.y + font_data.char_height;
    }

    if (cursor_active) {
        cursor_active <<= 1;
        draw_cursor();
    }

    uint32_t font_index = map_unicode_character(c);
    uint32_t font_x = (font_index % 16) * font_data.char_width;
    uint32_t font_y = (font_index / 16) * font_data.char_height;

    uint32_t* base_ptr = ((uint32_t*) (linear_framebuffer + text_position.y * vbe_mode_info.pitch)) + text_position.x;
    uint32_t* background_ptr = background_buffer + text_position.y * vbe_mode_info.width + text_position.x;
    uint8_t* font_ptr = font_data.data + font_y * font_data.image_width + font_x;
    for (uint32_t y = 0; y < font_data.char_height; y++) {
        for (uint32_t x = 0; x < font_data.char_width; x++) {
            uint8_t alpha = font_ptr[x];
            if (alpha == 0xFF) {
                base_ptr[x] = 0xFF000000 | color.rgb;
            } else if (alpha) {
                uint32_t bg = background_ptr[x];
                uint32_t r = (bg >> 16) & 0xFF;
                uint32_t g = (bg >> 8) & 0xFF;
                uint32_t b = bg & 0xFF;

                r = (r * (255 - alpha) + alpha * color.r) / 255;
                g = (g * (255 - alpha) + alpha * color.g) / 255;
                b = (b * (255 - alpha) + alpha * color.b) / 255;

                base_ptr[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
        base_ptr = (uint32_t*) (((void*) base_ptr) + vbe_mode_info.pitch);
        background_ptr += vbe_mode_info.width;
        font_ptr += font_data.image_width;
    }

    text_position.x += font_data.char_width;
    if (cursor_active >>= 1) {
        draw_cursor();
    }
}

void set_text_x(uint32_t x) {
    if (cursor_active) {
        cursor_active = 0;
        draw_cursor();
    }

    x = x * vbe_mode_info.width / 1000;
    text_position.x = x;
    text_position.rx = x;
}

void set_text_y(uint32_t y) {
    if (cursor_active) {
        cursor_active = 0;
        draw_cursor();
    }

    y = y * vbe_mode_info.height / 1000;
    text_position.y = y;
}

void clear_text() {
    clear_rect = dirty_rect;
    clear_y = dirty_rect.y0;
    clear_flag = 1;
}

__attribute__((optimize("O3")))
void tick_clear() {
    if (!clear_flag) {
        return;
    }

    void* target_ptr = linear_framebuffer + clear_y * vbe_mode_info.pitch + clear_rect.x0 * 4;
    uint32_t* background_ptr = background_buffer + clear_y * vbe_mode_info.width + clear_rect.x0;
    for (uint32_t y = clear_y; y < clear_y + font_data.char_height; y++) {
        memcpy(target_ptr, background_ptr, (clear_rect.x1 - clear_rect.x0) * 4);
        target_ptr += vbe_mode_info.pitch;
        background_ptr += vbe_mode_info.width;
    }

    clear_y += font_data.line_height;
    if (clear_y >= clear_rect.y1) {
        clear_flag = 0;
    }
}

void set_cursor_state(uint8_t state) {
    state = state > 0;
    if (cursor_active == state) {
        return;
    }

    cursor_active = state;
    draw_cursor();
}

void toggle_cursor_state() {
    set_cursor_state(!cursor_active);
}