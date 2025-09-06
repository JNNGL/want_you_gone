#include "irq.h"

#include <real/video.h>
#include <cpu/pic.h>

irq_handler_t irq_handlers[256];

void irq_set_handler(uint8_t vector, irq_handler_t handler) {
    irq_handlers[(vector + 32) & 0xFF] = handler;
}

__attribute__((regparm(1)))
void exception_handler(uint32_t id) {
    if (id >= 32) {
        irq_handler_t handler = irq_handlers[id];
        if (handler) {
            handler();
        }

        pic_send_eoi(id - 32);
        return;
    }

    for (int y = 0; y < vbe_mode_info.height; y++) {
        uint32_t* line = linear_framebuffer + y * vbe_mode_info.pitch;
        for (int x = 0; x < vbe_mode_info.width; x++) {
            line[x] = 0xFFFF << 16;
        }
    }

    asm volatile ("cli; hlt");
    while (1);
}