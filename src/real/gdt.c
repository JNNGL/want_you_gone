__asm__(".code16gcc");

#include "gdt.h"

void encode_gdt_entry(void* target, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags) {
    uint8_t* byte_ptr = (uint8_t*) target;

    byte_ptr[0] = limit & 0xFF;
    byte_ptr[1] = (limit >> 8) & 0xFF;
    byte_ptr[6] = (limit >> 16) & 0x0F;

    byte_ptr[2] = base & 0xFF;
    byte_ptr[3] = (base >> 8) & 0xFF;
    byte_ptr[4] = (base >> 16) & 0xFF;
    byte_ptr[7] = (base >> 24) & 0xFF;

    byte_ptr[5] = access_byte;

    byte_ptr[6] |= (flags << 4);
}