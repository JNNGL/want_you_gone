#pragma once

#include <stdint.h>

struct vbe_info_block {
    char signature[4];
    uint16_t version;
    uint16_t oem_string_ptr[2];
    uint8_t capabilities[4];
    uint16_t video_mode_ptr[2];
    uint16_t total_memory;
    uint8_t reserved[492];
} __attribute__((packed));

struct vbe_video_mode_info {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved[206];
} __attribute__((packed));

void query_vbe_info();
void query_video_mode_info(uint16_t mode);
void set_video_mode(uint16_t mode);
void enter_video_mode();

extern struct vbe_info_block vbe_info;
extern struct vbe_video_mode_info vbe_mode_info;
extern void* linear_framebuffer;