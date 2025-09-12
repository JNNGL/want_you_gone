#pragma once

#include <stdint.h>

struct executor_callbacks {
    void(*putchar)(char);
    void(*clear)();
    void(*set_x)(uint32_t);
    void(*set_y)(uint32_t);
};

extern struct executor_callbacks executor_cb;

void add_executor(char* buffer);
void tick_executors();