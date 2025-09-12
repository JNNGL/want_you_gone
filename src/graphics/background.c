#include "background.h"

#include <real/video.h>
#include <util/lib.h>

uint32_t* background_buffer = 0;

static uint32_t mix(uint32_t color1, uint32_t color2, uint32_t t) {
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

void render_background() {
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

    {
        uint32_t x0 = vbe_mode_info.width * 3 / 100;
        uint32_t x1 = vbe_mode_info.width * 97 / 100;
        uint32_t y0 = vbe_mode_info.height * 4 / 100;
        uint32_t y1 = vbe_mode_info.height * 96 / 100;

        for (uint32_t x = x0; x <= x1; x++) {
            background_buffer[y0 * vbe_mode_info.width + x] = 0xFF6C582C;
            background_buffer[(y0 + 1) * vbe_mode_info.width + x] = 0xFF6C582C;
            background_buffer[y1 * vbe_mode_info.width + x] = 0xFF6C582C;
            background_buffer[(y1 - 1) * vbe_mode_info.width + x] = 0xFF6C582C;
        }
        for (uint32_t y = y0; y <= y1; y++) {
            background_buffer[y * vbe_mode_info.width + x0] = 0xFF6C582C;
            background_buffer[y * vbe_mode_info.width + x0 + 1] = 0xFF6C582C;
            background_buffer[y * vbe_mode_info.width + x1] = 0xFF6C582C;
            background_buffer[y * vbe_mode_info.width + x1 + 1] = 0xFF6C582C;
        }
    }
}

__attribute__((optimize("O3")))
void copy_background() {
    for (uint32_t y = 0; y < vbe_mode_info.height; y++) {
        memcpy(linear_framebuffer + y * vbe_mode_info.pitch, background_buffer + y * vbe_mode_info.width, 4 * vbe_mode_info.width);
    }
}

