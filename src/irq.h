#pragma once

#include <stdint.h>

typedef void(*irq_handler_t)();

void irq_set_handler(uint8_t vector, irq_handler_t handler);

extern irq_handler_t irq_handlers[256];