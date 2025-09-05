#include "idt.h"

#include <irq.h>

static uint8_t vectors[256];
extern void* isr_stub_table[];

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt_data[vector];

    descriptor->isr_low = (uint32_t)(uintptr_t) isr & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->attributes = flags;
    descriptor->isr_high = (uint32_t)(uintptr_t) isr >> 16;
    descriptor->reserved = 0;
}

void idt_init() {
    idtr.base = (uintptr_t) &idt_data[0];
    idtr.limit = (uint16_t) sizeof(idt_entry_t) * 256 - 1;

    for (uint32_t i = 0; i < 256; i++) {
        irq_handlers[i] = 0;
    }

    for (uint8_t vector = 0; vector < 64; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = 1;
    }

    asm volatile("lidt %0" : : "m"(idtr));
}

__attribute__((aligned(0x10)))
idt_entry_t idt_data[256];
idtr_t idtr;