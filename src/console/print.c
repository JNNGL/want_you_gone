#include "print.h"

#include <real/video.h>
#include <console/psf.h>

static struct {
    psf1_header_t header;
    uint8_t* glyph_data;
} console_font;

static struct {
    int x;
    int y;
} text_position;

void init_console(void* font) {
    load_font(font);
    text_position.x = 0;
    text_position.y = 0;
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
    if (ch == '\n') {
        text_position.y += console_font.header.char_size;
        if (text_position.y >= vbe_mode_info.height) {
            text_position.y = 0;
        }
        text_position.x = 0;
        return;
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
    while (*str != 0) {
        putc(*str++);
    }
}

void puts(const char* str) {
    kputs(str);
    putc('\n');
}