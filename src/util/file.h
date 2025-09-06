#pragma once

#include <stdint.h>

struct file_info {
    uint32_t size;
    void* data;
};

struct file_info find_file(const char* filename);