#pragma once

#include <stdint.h>

void pit_set_frequency(int freq);
void pit_init(int freq);

extern uint32_t pit_ticks;