#include <real/video.h>
#include <console/print.h>
#include <device/pci.h>
#include <device/hda.h>
#include <device/pit.h>
#include <util/file.h>
#include <util/lib.h>
#include <cpu/idt.h>
#include <cpu/irq.h>
#include <cpu/pic.h>

#include <graphics/background.h>
#include <graphics/font.h>
#include <executor.h>

#define PIT_FREQUENCY 100

static struct file_info sound_file;

static uint32_t current_time = 0;
static uint8_t refilling_sound_buffer = 0;
static uint32_t write_position = 0;
static uint32_t read_position = 0;
static uint8_t pending_playback = 0;
static uint8_t cursor_blink = 0;

void refill_sound_buffer() {
    if (current_time < 10) {
        return;
    }

    uint32_t copy_size = hda_buffer_size - write_position;
    if (882 < copy_size) {
        copy_size = 882;
    }

    memcpy(hda_data_buffer + write_position, sound_file.data + read_position, copy_size);
    asm volatile("wbinvd");

    read_position += copy_size;
    write_position += copy_size;
    if (write_position >= hda_buffer_size) {
        write_position = 0;
    }

    if (read_position >= sound_file.size) {
        refilling_sound_buffer = 0;
        hda_stop_playback();
        puts("finished playback");
    }
}

void pit_handler() {
    if (pending_playback) {
        pending_playback = 0;
        hda_start_transfer();
        return;
    }

    if (refilling_sound_buffer) {
        refill_sound_buffer();
    }

    tick_executors();
    tick_clear();

    if (cursor_blink && !(current_time % 30)) {
        toggle_cursor_state();
    }

    current_time++;
}

void set_blink_state(uint8_t state) {
    cursor_blink = state;
    if (!state) {
        set_cursor_state(0);
    }
}

__attribute__((noreturn, optimize("O3")))
void pmain() {
    idt_init();
    pic_remap();
    asm volatile("sti");

    init_console(find_file("zap-vga09.psf").data);

    puts("Hello, world!");

    pit_init(PIT_FREQUENCY);

    sound_file = find_file("song.pcm");
    if (!sound_file.data) {
        puts("couldn't find file");
    }

    pci_check_buses();

    hda_alloc_data_buffer(44100 * 2);
    hda_prepare();

    memcpy(hda_data_buffer, sound_file.data, hda_buffer_size);
    asm volatile("wbinvd");

    read_position = hda_buffer_size;
    refilling_sound_buffer = 1;
    pending_playback = 1;

    console_disable();

    init_font(find_file("consolas36.raw").data);

    render_background();
    copy_background();

    executor_cb.putchar = draw_char;
    executor_cb.clear = clear_text;
    executor_cb.set_x = set_text_x;
    executor_cb.set_y = set_text_y;
    executor_cb.blink = set_blink_state;

    add_executor(find_file("lyrics.txt").data);

    irq_set_handler(0, pit_handler);

    while (1);
}