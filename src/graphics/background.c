#include "background.h"

#include <graphics/font.h>
#include <real/video.h>
#include <util/file.h>
#include <util/lib.h>

uint32_t* background_buffer = 0;
uint8_t* window_buffer = 0;

static uint32_t wy0, wy1;
static uint32_t w0x0, w0x1;

static uint32_t rng_state;

__attribute__((optimize("O3")))
inline static uint32_t mix(uint32_t color1, uint32_t color2, uint32_t t) {
    if (t == 255) {
        return color2;
    } else if (t == 0) {
        return color1;
    }

    uint32_t r1 = (color1 >> 16) & 0xFF;
    uint32_t g1 = (color1 >> 8) & 0xFF;
    uint32_t b1 = color1 & 0xFF;

    uint32_t r2 = (color2 >> 16) & 0xFF;
    uint32_t g2 = (color2 >> 8) & 0xFF;
    uint32_t b2 = color2 & 0xFF;

    r1 = (r1 * (255 - t) + t * r2) / 255;
    g1 = (g1 * (255 - t) + t * g2) / 255;
    b1 = (b1 * (255 - t) + t * b2) / 255;

    return (0xFF << 24) | (r1 << 16) | (g1 << 8) | b1;
}

static int bayer[16] = {
        0, 12, 3, 15,
        8, 4, 11, 7,
        2, 14, 1, 13,
        10, 6, 9, 5
};

uint32_t pcg_hash() {
    rng_state = rng_state * 747796405u + 2891336453u;
    uint32_t state = rng_state;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void render_background() {
    rng_state = 0x58394579;

    if (background_buffer == 0) {
        background_buffer = malloca(4 * vbe_mode_info.width * vbe_mode_info.height, 8);
    }

    int center_x = vbe_mode_info.width / 2;
    int center_y = vbe_mode_info.height / 2;

    for (int y = 0; y < vbe_mode_info.height; y++) {
        for (int x = 0; x < vbe_mode_info.width; x++) {
            int dx = 2 * (x - center_x);
            int dy = 2 * (y - center_y);
            if (dx > vbe_mode_info.width) {
                dx = vbe_mode_info.width;
            } else if (dx < -vbe_mode_info.width) {
                dx = -vbe_mode_info.width;
            }
            if (dy > vbe_mode_info.height) {
                dy = vbe_mode_info.height;
            } else if (dy < -vbe_mode_info.height) {
                dy = -vbe_mode_info.height;
            }

            dx = dx * dx / vbe_mode_info.width;
            dx = dx * dx / vbe_mode_info.width;

            dy = dy * dy / vbe_mode_info.height;
            dy = dy * dy / vbe_mode_info.height;

            int dither = bayer[((y & 3) << 2) | (x & 3)] - 8;

            int alpha = dx * 160 / vbe_mode_info.width + dy * 128 / vbe_mode_info.height + dither;
            if (alpha < 0) {
                alpha = 0;
            } else if (alpha > 255) {
                alpha = 255;
            }

            background_buffer[y * vbe_mode_info.width + x] = mix(0xFF60421A, 0xFF2D1C0B, alpha);
        }
    }

    wy0 = vbe_mode_info.height * 3 / 100;
    wy1 = wy0 + vbe_mode_info.width * 10 / 100;
    uint32_t w2x0 = vbe_mode_info.width * 85 / 100;
    uint32_t w2x1 = vbe_mode_info.width * 95 / 100;
    uint32_t w1x1 = w2x0 - vbe_mode_info.width * 1 / 100;
    uint32_t w1x0 = w1x1 - get_font_width() * 5;
    w0x1 = w1x0 - vbe_mode_info.width * 1 / 100;
    w0x0 = w0x1 - vbe_mode_info.width * 20 / 100;

    if (window_buffer == 0) {
        window_buffer = calloca((w0x1 - w0x0) / 2 * (wy1 - wy0) * 5, 8);
    }

    {
        uint32_t buffer_width = (w0x1 - w0x0) / 2;
        uint32_t block_width = (buffer_width - 20) / 6;
        for (uint32_t block_x = 0; block_x < 6; block_x++) {
            uint32_t count = 0;
            uint8_t set = pcg_hash() & 1;
            for (uint32_t y = 0; y < (wy1 - wy0) * 5; y += 5) {
                if (count-- == 0) {
                    count = pcg_hash() % 5;
                    set ^= 1;
                }
                for (uint32_t dx = (block_width * 2 + 9) / 10; dx < block_width * 8 / 10; dx++) {
                    window_buffer[y * buffer_width + block_x * block_width + dx + 10] = set;
                    window_buffer[(y + 1) * buffer_width + block_x * block_width + dx + 10] = set;
                    window_buffer[(y + 2) * buffer_width + block_x * block_width + dx + 10] = set;
                }
            }
        }
    }

    {
        const uint32_t border_color = 0xFF6C582C;

        uint32_t x0 = vbe_mode_info.width * 3 / 100;
        uint32_t x1 = vbe_mode_info.width * 97 / 100;
        uint32_t y0 = vbe_mode_info.height * 4 / 100;
        uint32_t y1 = vbe_mode_info.height * 96 / 100;

        for (uint32_t d = 0; d < 3; d++) {
            for (uint32_t x = x0; x <= x1; x++) {
                if ((x >= w2x0 && x <= w2x1) || (x >= w1x0 && x <= w1x1) || (x >= w0x0 && x <= w0x1)) {
                    background_buffer[(wy0 + d) * vbe_mode_info.width + x] = border_color;
                    background_buffer[(wy1 - d) * vbe_mode_info.width + x] = border_color;
                } else {
                    background_buffer[(y0 + d) * vbe_mode_info.width + x] = border_color;
                }

                background_buffer[(y1 - d) * vbe_mode_info.width + x] = border_color;
            }
            for (uint32_t y = y0; y <= y1; y++) {
                background_buffer[y * vbe_mode_info.width + x0 + d] = border_color;
                background_buffer[y * vbe_mode_info.width + x1 - d] = border_color;
            }
            for (uint32_t y = wy0; y <= wy1; y++) {
                background_buffer[y * vbe_mode_info.width + w2x0 + d] = border_color;
                background_buffer[y * vbe_mode_info.width + w2x1 - d] = border_color;

                background_buffer[y * vbe_mode_info.width + w1x0 + d] = border_color;
                background_buffer[y * vbe_mode_info.width + w1x1 - d] = border_color;

                background_buffer[y * vbe_mode_info.width + w0x0 + d] = border_color;
                background_buffer[y * vbe_mode_info.width + w0x1 - d] = border_color;
            }
        }
    }

    struct file_info logo = find_file("logo.raw");
    uint32_t logo_width = ((uint32_t*) logo.data)[0];
    uint32_t logo_height = ((uint32_t*) logo.data)[1];
    uint8_t* logo_data = logo.data + 8;

    {
        w2x0 += 4 + vbe_mode_info.width / 100;
        w2x1 -= 6 + vbe_mode_info.width / 100;
        uint32_t lwy0 = wy0 + 4 + vbe_mode_info.width / 100;
        uint32_t lwy1 = wy1 - 6 - vbe_mode_info.width / 100;

        uint32_t prev_ly = 0;
        for (int y = lwy0; y < lwy1; y++) {
            uint32_t prev_lx = 0;
            uint32_t ly = (y - lwy0) * logo_height / (lwy1 - lwy0);
            for (int x = w2x0; x < w2x1; x++) {
                uint32_t lx = (x - w2x0) * logo_width / (w2x1 - w2x0);

                uint32_t alpha = 0, count = 0;
                for (uint32_t s_ly = prev_ly; s_ly <= ly; s_ly++) {
                    for (uint32_t s_lx = prev_lx; s_lx <= lx; s_lx++) {
                        alpha += logo_data[s_ly * logo_width + s_lx];
                        count++;
                    }
                }
                alpha /= count;

                uint32_t* background_ptr = &background_buffer[y * vbe_mode_info.width + x];
                *background_ptr = mix(*background_ptr, 0x794E23, alpha);

                prev_lx = lx;
            }
            prev_ly = ly;
        }
    }

    for (uint32_t y = 0; y < vbe_mode_info.height; y++) {
        memcpy(linear_framebuffer + y * vbe_mode_info.pitch, background_buffer + y * vbe_mode_info.width, 4 * vbe_mode_info.width);
    }

    {
        set_text_x(840 - get_font_width() * 4500 / vbe_mode_info.width);
        set_text_y(36);

        const char *str = "2.67\n1002\n45.6";
        while (*str) {
            draw_char(*str);
            str++;
        }

        reset_text_dirty_rect();

        set_text_x(0);
        set_text_y(0);
    }
}

__attribute__((optimize("O3")))
void tick_background() {
    static uint32_t w0_y_offset = 0x0FFFFFFF, w1_y_offset = 100;

    static uint8_t flag = 0;
    if ((flag ^= 1) & 1) {
        return;
    }

    if ((flag ^= 2) & 2) {
        w0_y_offset -= 2;
        w1_y_offset += 2;
    } else {
        w0_y_offset -= 3;
        w1_y_offset += 3;
    }

    uint32_t buffer_width = (w0x1 - w0x0) / 2;
    uint32_t buffer_height = (wy1 - wy0) * 5;
    for (uint32_t y = wy0 + 3; y < wy1 - 3; y++) {
        uint8_t* window_base0 = &window_buffer[((y + w0_y_offset - wy0) % buffer_height) * buffer_width];
        uint8_t* window_base1 = &window_buffer[((y + w1_y_offset - wy0) % buffer_height) * buffer_width];
        uint32_t* buffer_base = linear_framebuffer + y * vbe_mode_info.pitch;
        uint32_t* background_base = background_buffer + y * vbe_mode_info.width;
        uint32_t alpha = 255;
        if (y - 3 - wy0 < buffer_height / 30) {
            alpha = (y - 3 - wy0) * 255 / (buffer_height / 30);
        }
        for (uint32_t x = w0x0; x < w0x1; x++) {
            uint32_t start = w0x0;
            uint8_t* window = window_base0;
            if (x >= w0x0 + buffer_width) {
                start = w0x0 + buffer_width;
                window = window_base1;
            }

            uint32_t background = background_base[x];
            if (window[x - start]) {
                buffer_base[x] = mix(background, 0x806439, alpha);
            } else {
                buffer_base[x] = background;
            }
        }
    }
}

