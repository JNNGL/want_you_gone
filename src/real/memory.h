#pragma once

#include <stdint.h>

struct memory_map_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
};

void detect_memory();
void print_memory_map();

extern struct memory_map_entry* memory_map;
extern uint32_t* memory_map_length;