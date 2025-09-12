__asm__(".code16gcc");

#include <real/video.h>
#include <real/memory.h>
#include <real/data.h>
#include <real/util.h>
#include <real/gdt.h>
#include <stdint.h>
#include <stddef.h>

__attribute__((optimize("O3")))
static void memcpy(void* dest, void* src, uint32_t count) {
    if (!count) return;
    while (count >= 4) { *(uint32_t*) dest = *(uint32_t*) src; dest += 4; src += 4; count -= 4; }
    while (count >= 2) { *(uint16_t*) dest = *(uint16_t*) src; dest += 2; src += 2; count -= 2; }
    while (count >= 1) { *(uint8_t*) dest = *(uint8_t*) src; dest += 1; src += 1; count -= 1; }
}

__attribute__((regparm(2)))
extern void protected_mode(uint16_t limit, void* base);

extern uint32_t boot_file_size;
extern uint32_t data_file_size;

uint64_t gdt_data[5];

extern volatile uint8_t dap;
extern volatile uint8_t bios_disk;
extern volatile uint16_t dap_num_sectors;
extern volatile uint16_t dap_buf_offset;
extern volatile uint16_t dap_buf_segment;
extern volatile uint32_t dap_lba_lower;
extern volatile uint32_t dap_lba_upper;
extern uint8_t disk_buffer[];

void* data_file_ptr;

void read_disk_sector(uint32_t sector, void* target) {
    dap_num_sectors = 1;
    dap_buf_offset = (uint32_t)(uintptr_t) &disk_buffer & 0xFFFF;
    dap_buf_segment = (uint32_t)(uintptr_t) &disk_buffer >> 16;
    dap_lba_lower = sector * dap_num_sectors;
    dap_lba_upper = 0;

    asm("int $0x13" : : "a"(0x4200), "d"(bios_disk), "D"((uint16_t)(uintptr_t) &dap));

    memcpy(target, disk_buffer, 512);
}

_Noreturn
void boot_main() {
    detect_memory();
    print_memory_map();

    puts16("Loading data from disk...");

    data_file_ptr = free_memory_ptr;

    for (uint32_t offset = 0; offset < (data_file_size + 511) / 512; offset++) {
        read_disk_sector((boot_file_size + 511) / 512 + offset, free_memory_ptr);
        free_memory_ptr += 512;
    }

    enter_video_mode();

    encode_gdt_entry(&gdt_data[0], 0, 0, 0, 0);
    encode_gdt_entry(&gdt_data[1], 0, 0xFFFFF, 0x9A, 0xC);
    encode_gdt_entry(&gdt_data[2], 0, 0xFFFFF, 0x92, 0xC);
    encode_gdt_entry(&gdt_data[3], 0, 0xFFFFF, 0xFA, 0xC);
    encode_gdt_entry(&gdt_data[4], 0, 0xFFFFF, 0xF2, 0xC);

    protected_mode(sizeof(gdt_data) - 1, gdt_data);

    while (1);
}