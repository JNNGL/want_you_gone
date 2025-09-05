__asm__(".code16gcc");

#include "video.h"

#include <real/util.h>

void query_vbe_info() {
    uint16_t status;
    asm volatile("int $0x10" : "=a"(status) : "a"(0x4F00), "D"((uintptr_t) &vbe_info));

    if (status != 0x004F) {
        die16("Failed to query VBE info.");
    }
}

void query_video_mode_info(uint16_t mode) {
    uint16_t status;
    asm volatile("int $0x10" : "=a"(status) : "a"(0x4F01), "c"(mode), "D"((uintptr_t) &vbe_mode_info));

    if (status != 0x004F) {
        die16("Failed to query video mode info.");
    }
}

void set_video_mode(uint16_t mode) {
    uint16_t status;
    asm volatile("int $0x10;" : "=a"(status) : "a"(0x4F02), "b"(mode));

    if (status != 0x004F) {
        die16("Failed to set video mode.");
    }
}

void enter_video_mode() {
    query_vbe_info();

    uint32_t vbe_addr = ((uint32_t) vbe_info.video_mode_ptr[1] << 4) | vbe_info.video_mode_ptr[0];

    uint16_t best_mode = 0;
    uint32_t best_resolution = 0;
    for (uint16_t* i = (uint16_t*)(uintptr_t) vbe_addr; *i != 0xFFFF; i++) {
        query_video_mode_info(*i);
        if (!(vbe_mode_info.attributes & (1 << 7)) || vbe_mode_info.bpp != 32) {
            continue;
        }

        int resolution = vbe_mode_info.width * vbe_mode_info.height;
        if (resolution > 1280 * 720) {
            continue;
        }

        if (resolution > best_resolution) {
            best_resolution = resolution;
            best_mode = *i;
        }
    }

    if (best_resolution) {
        query_video_mode_info(best_mode);
        set_video_mode(best_mode | 0x4000);
        linear_framebuffer = (void*)(uintptr_t) vbe_mode_info.framebuffer;
    } else {
        die16("Couldn't find suitable video mode.");
    }
}

struct vbe_info_block vbe_info = {};
struct vbe_video_mode_info vbe_mode_info = {};
void* linear_framebuffer = 0;