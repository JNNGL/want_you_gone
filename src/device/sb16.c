#include "sb16.h"

#include <console/print.h>
#include <device/pit.h>
#include <cpu/io.h>
#include <cpu/irq.h>

#define DSP_PORT_MIXER       0x224
#define DSP_PORT_MIXER_DATA  0x225
#define DSP_PORT_RESET       0x226
#define DSP_PORT_READ        0x22A
#define DSP_PORT_WRITE       0x22C
#define DSP_PORT_READ_STATUS 0x22E
#define DSP_PORT_16IACK      0x22F

#define DSP_CMD_SET_TIME        0x40
#define DSP_CMD_SET_SAMPLE_RATE 0x41
#define DSP_CMD_SPEAKER_ON      0xD1
#define DSP_CMD_SPEAKER_OFF     0xD2
#define DSP_CMD_STOP_8BIT       0xD0
#define DSP_CMD_RESUME_8BIT     0xD4
#define DSP_CMD_STOP_16BIT      0xD5
#define DSP_CMD_RESUME_16BIT    0xD6
#define DSP_CMD_GET_VERSION     0xE1

#define DSP_CMD_MIXER_VOLUME    0x22
#define DSP_CMD_MIXER_SET_IRQ   0x80

uintptr_t sb16_buffer_ptr;
uint32_t sb16_buffer_size;

void sb16_program_dma() {
    outb(0x0A, 5);
    outb(0x0C, 1);
    outb(0x0B, 0x49);
    outb(0x83, sb16_buffer_ptr >> 16);
    outb(0x02, sb16_buffer_ptr & 0xFF);
    outb(0x02, (sb16_buffer_ptr >> 8) & 0xFF);
    outb(0x03, 0xFF);
    outb(0x03, 0xFF);
    outb(0x0A, 1);
}

void sb16_start_playback() {
    uint32_t sample_rate = 8000;
    outb(DSP_PORT_WRITE, DSP_CMD_SET_SAMPLE_RATE);
    outb(DSP_PORT_WRITE, sample_rate >> 8);
    outb(DSP_PORT_WRITE, sample_rate & 0xFF);

    outb(DSP_PORT_WRITE, 0xC0);
    outb(DSP_PORT_WRITE, 0);
    outb(DSP_PORT_WRITE, 0xFF);
    outb(DSP_PORT_WRITE, 0xFF);
}

void sb16_irq_handler() {
    puts("int");

    sb16_buffer_ptr += 0xFFFF;

    outb(DSP_PORT_MIXER, DSP_CMD_MIXER_SET_IRQ);
    outb(DSP_PORT_MIXER_DATA, 2);

    sb16_program_dma();
    sb16_start_playback();
}

void sb16_init() {
    outb(DSP_PORT_RESET, 1);

    pit_ticks = 0;
    while (pit_ticks < 5);

    outb(DSP_PORT_RESET, 0);
    if (inb(DSP_PORT_READ) != 0xAA) {
        return;
    }

    puts("found sb16 card");

    irq_set_handler(5, sb16_irq_handler);

    outb(DSP_PORT_MIXER, DSP_CMD_MIXER_SET_IRQ);
    outb(DSP_PORT_MIXER_DATA, 2);

    outb(DSP_PORT_MIXER, DSP_CMD_MIXER_VOLUME);
    outb(DSP_PORT_MIXER_DATA, 0xFF);

    outb(DSP_PORT_WRITE, DSP_CMD_SPEAKER_ON);

    sb16_program_dma();
    sb16_start_playback();
}