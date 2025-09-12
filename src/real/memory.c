__asm__(".code16gcc");

#include "memory.h"

#include <real/util.h>

extern uint32_t boot_file_size;

void detect_memory() {
    uint8_t carry = 0;
    uint32_t eax = 0;
    uint32_t ebx = 0;
    uint32_t i = 0;

    *memory_map_length = 0;

    memory_map[i].attributes = 1;
    asm volatile("int $0x15"
            : "=a"(eax), "=b"(ebx), "=@ccc"(carry)
            : "a"(0xE820ul), "b"(ebx), "c"(24ul), "d"(0x534D4150), "D"((uintptr_t) (memory_map + i)));

    if (eax != 0x534D4150 || carry) {
        die16("First INT 0x15, EAX = 0xE820 call was unsuccessful.");
    }

    if (ebx == 0) {
        *memory_map_length = 1;
        return;
    }

    while (!carry && ebx) {
        i++;

        memory_map[i].attributes = 1;
        asm volatile("int $0x15"
                : "=a"(eax), "=b"(ebx), "=@ccc"(carry)
                : "a"(0xE820ul), "b"(ebx), "c"(24ul), "d"(0x534D4150), "D"((uintptr_t) (memory_map + i)));
    }

    *memory_map_length = i;

    uint32_t code_end = boot_file_size + 0x7C00;

    uint32_t best_index = 0;
    uint64_t largest_size = 0;
    for (uint32_t i = 0; i < *memory_map_length; i++) {
        struct memory_map_entry entry = memory_map[i];
        if ((entry.attributes & 1) == 0 || entry.type != 1) {
            continue;
        }

        if (entry.base >= 0xFFFFFFFF) {
            continue;
        }

        if (entry.base + entry.length > 0xFFFFFFFF) {
            entry.length = 0xFFFFFFFF - entry.base;
        }

        if (entry.base + entry.length < code_end) {
            continue;
        }

        if (entry.base <= code_end) {
            entry.base = code_end + 1;
        }

        if (entry.length > largest_size) {
            largest_size = entry.length;
            best_index = i;
        }
    }

    if (largest_size == 0) {
        die16("failed to find suitable memory segment");
    }

    free_memory_ptr = (void*)(uintptr_t) memory_map[best_index].base;
}

void print_memory_map() {
    puts16("Memory map: ");

    for (uint32_t i = 0; i < *memory_map_length; i++) {
        kputs16("0x");
        print64h16(memory_map[i].base);
        kputs16(" - 0x");
        print64h16(memory_map[i].base + memory_map[i].length);
        kputs16(" | ");
        putc16("0123456789"[memory_map[i].type]);
        putc16(' ');
        putc16((memory_map[i].attributes & 1) ? '1' : '0');
        puts16("");
    }
}

struct memory_map_entry* memory_map = (struct memory_map_entry*)(uintptr_t) 0x508;
uint32_t* memory_map_length = (uint32_t*)(uintptr_t) 0x500;

void* free_memory_ptr;