#pragma once

#include <stdint.h>

#define PSF1_FONT_MAGIC 0x0436

typedef struct {
    uint16_t magic;
    uint8_t font_mode;
    uint8_t char_size;
} __attribute__((packed)) psf1_header_t;

