#include "pit.h"

#include <cpu/io.h>
#include <cpu/irq.h>

void pit_irq_handler() {
    pit_ticks++;
}

void pit_set_frequency(int freq) {
    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
}

void pit_init(int freq) {
    pit_ticks = 0;

    pit_set_frequency(freq);
    irq_set_handler(0, pit_irq_handler);
}

uint32_t pit_ticks = 0;