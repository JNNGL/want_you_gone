#include <real/video.h>
#include <console/print.h>
#include <util/file.h>
#include <cpu/idt.h>
#include <cpu/pic.h>

__attribute__((noreturn, optimize("O3")))
void pmain() {
    idt_init();
    pic_remap();
    asm volatile("sti");

    init_console(find_file("zap-vga09.psf").data);

    puts("Hello, world!");

    while (1);
}