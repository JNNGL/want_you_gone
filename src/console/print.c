#include "print.h"

#include <real/video.h>
#include <console/psf.h>
#include <util/lib.h>

static struct {
    psf1_header_t header;
    uint8_t* glyph_data;
} console_font;

static struct {
    int x;
    int y;
} text_position;

static struct {
    int x0;
    int y0;
    int x1;
    int y1;
} dirty_rect;

static uint8_t enabled = 0;

void init_console(void* font) {
    load_font(font);
    text_position.x = 0;
    text_position.y = 0;
    dirty_rect.x0 = 0;
    dirty_rect.y0 = 0;
    dirty_rect.x1 = 0;
    dirty_rect.y1 = 0;
    enabled = 1;
}

void load_font(void* ptr) {
    console_font.header = *(psf1_header_t*) ptr;
    console_font.glyph_data = (uint8_t*) ptr + sizeof(psf1_header_t);
}

static uint8_t* get_glyph_pointer(char ch) {
    return console_font.glyph_data + ch * console_font.header.char_size;
}

__attribute__((optimize("O3")))
void putc(char ch) {
    if (!enabled) {
        return;
    }

    if (ch == '\n') {
        text_position.y += console_font.header.char_size;
        if (text_position.y >= vbe_mode_info.height) {
            text_position.y = 0;
        }
        text_position.x = 0;
        return;
    }

    if (text_position.x < dirty_rect.x0) {
        dirty_rect.x0 = text_position.x;
    }
    if (text_position.y < dirty_rect.y0) {
        dirty_rect.y0 = text_position.y;
    }
    if (text_position.x + 8 > dirty_rect.x1) {
        dirty_rect.x1 = text_position.x + 8;
    }
    if (text_position.y + console_font.header.char_size > dirty_rect.y1) {
        dirty_rect.y1 = text_position.y + console_font.header.char_size;
    }

    uint8_t* data = get_glyph_pointer(ch);
    for (int y = 0; y < console_font.header.char_size; y++) {
        uint32_t* target = ((uint32_t*) (linear_framebuffer + (y + text_position.y) * vbe_mode_info.pitch)) + text_position.x;
        uint8_t font_line = data[y];
        for (int x = 0; x < 8; x++) {
            target[x] = ((font_line >> (7 - x)) & 1) ? 0xFFFFFFFF : 0xFF000000;
        }
    }

    text_position.x += 8;
    if (text_position.x >= vbe_mode_info.width) {
        text_position.x = 0;
        text_position.y += console_font.header.char_size;
        if (text_position.y >= vbe_mode_info.height) {
            text_position.y = 0;
        }
    }
}

void kputs(const char* str) {
    if (!enabled) {
        return;
    }

    while (*str != 0) {
        putc(*str++);
    }
}

void puts(const char* str) {
    if (!enabled) {
        return;
    }

    kputs(str);
    putc('\n');
}

__attribute__((optimize("O3")))
void clear_console() {
    if (!enabled) {
        return;
    }

    if (dirty_rect.x0 == dirty_rect.x1 || dirty_rect.y0 == dirty_rect.y1) {
        return;
    }

    for (int y = dirty_rect.y0; y < dirty_rect.y1; y++) {
        memset(linear_framebuffer + dirty_rect.x0 * 4 + vbe_mode_info.pitch * y, 0, (dirty_rect.x1 - dirty_rect.x0) * 4);
    }
}

void console_set_x(uint32_t x) {
    text_position.x = x;
}

void console_set_y(uint32_t y) {
    text_position.y = y;
}

void console_disable() {
    enabled = 0;
}

void console_enable() {
    enabled = 1;
}