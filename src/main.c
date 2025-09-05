__asm__(".code32");

#include <real/video.h>

__attribute__((optimize("O3")))
void pmain() {
    uint8_t b = 0;
    while (1) {
        for (int y = 0; y < vbe_mode_info.height; y++) {
            uint32_t* line = linear_framebuffer + y * vbe_mode_info.pitch;
            for (int x = 0; x < vbe_mode_info.width; x++) {
                uint32_t r = x & 0xFF;
                uint32_t g = y & 0xFF;
                line[x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
        b++;
    }
}