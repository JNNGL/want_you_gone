__asm__(".code16gcc");

#include "memory.h"

#include <real/util.h>

void detect_memory() {
    uint8_t carry = 0;
    uint32_t eax = 0;
    uint32_t ebx = 0;
    uint32_t i = 0;

    *((uint32_t*)(uintptr_t) 0x500) = 0;

    memory_map[i].attributes = 1;
    asm volatile("int $0x15"
            : "=a"(eax), "=b"(ebx), "=@ccc"(carry)
            : "a"(0xE820ul), "b"(ebx), "c"(24ul), "d"(0x534D4150), "D"((uintptr_t) (memory_map + i)));

    if (eax != 0x534D4150 || carry) {
        die16("First INT 0x15, EAX = 0xE820 call was unsuccessful.");
    }

    if (ebx == 0) {
        *((uint32_t*)(uintptr_t) 0x500) = 1;
        return;
    }

    while (!carry && ebx) {
        i++;

        memory_map[i].attributes = 1;
        asm volatile("int $0x15"
                : "=a"(eax), "=b"(ebx), "=@ccc"(carry)
                : "a"(0xE820ul), "b"(ebx), "c"(24ul), "d"(0x534D4150), "D"((uintptr_t) (memory_map + i)));
    }

    *((uint32_t*)(uintptr_t) 0x500) = i;
}

void print_memory_map() {
    for (uint32_t i = 0; i < *memory_map_length; i++) {
        fputs16("0x");
        print64h16(memory_map[i].base);
        fputs16(" - 0x");
        print64h16(memory_map[i].base + memory_map[i].length);
        fputs16(" | ");
        putc16("0123456789"[memory_map[i].type]);
        putc16(' ');
        putc16((memory_map[i].attributes & 1) ? '1' : '0');
        puts16("");
    }
}

struct memory_map_entry* memory_map = (struct memory_map_entry*)(uintptr_t) 0x508;
uint32_t* memory_map_length = (uint32_t*)(uintptr_t) 0x500;
