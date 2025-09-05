#pragma once

#include <stdint.h>

void encode_gdt_entry(void* target, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags);