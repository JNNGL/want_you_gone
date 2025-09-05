__asm__(".code16gcc");

#include <real/video.h>
#include <real/memory.h>
#include <real/util.h>
#include <real/gdt.h>
#include <stdint.h>

__attribute__((regparm(2)))
extern void protected_mode(uint16_t limit, void* base);

uint64_t gdt_data[5];

_Noreturn
void boot_main() {
    detect_memory();
    print_memory_map();

    enter_video_mode();

    encode_gdt_entry(&gdt_data[0], 0, 0, 0, 0);
    encode_gdt_entry(&gdt_data[1], 0, 0xFFFFF, 0x9A, 0xC);
    encode_gdt_entry(&gdt_data[2], 0, 0xFFFFF, 0x92, 0xC);
    encode_gdt_entry(&gdt_data[3], 0, 0xFFFFF, 0xFA, 0xC);
    encode_gdt_entry(&gdt_data[4], 0, 0xFFFFF, 0xF2, 0xC);

    protected_mode(sizeof(gdt_data) - 1, gdt_data);

    while (1);
}