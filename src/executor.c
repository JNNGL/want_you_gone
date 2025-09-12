#include "executor.h"

#include <cpu/io.h>
#include <util/lib.h>

struct executor_callbacks executor_cb;

typedef struct {
    uint8_t active;
    uint32_t sleep;
    char* buffer;
    uint32_t index;
    uint32_t period;
    uint32_t prev_char;
} executor_t;

static uint32_t current_time = 0;

static executor_t* executors = 0;
static uint32_t executor_count = 0;

void add_executor(char* buffer) {
    if (!executors) {
        executors = calloc(sizeof(executor_t) * 16);
    }

    executor_t* executor = &executors[executor_count++];
    executor->buffer = buffer;
    executor->index = 0;
    executor->active = 1;
}

static char fetch_char(executor_t* executor) {
    return executor->buffer[executor->index++];
}

static char peek_char(executor_t* executor) {
    return executor->buffer[executor->index];
}

static uint32_t parse_uint(executor_t* executor) {
    while (!isdigit(peek_char(executor))) {
        fetch_char(executor);
    }

    uint32_t result = 0;
    while (isdigit(peek_char(executor))) {
        result *= 10;
        result += fetch_char(executor) - '0';
    }

    return result;
}

static void execute_instruction(executor_t* executor) {
    if (fetch_char(executor) != '{') {
        return;
    }

    char command = fetch_char(executor);
    switch (command) {
        case 'e':
            executor->active = 0;
            return;

        case 's':
            executor->sleep = current_time + parse_uint(executor);
            break;

        case 'p':
            executor->period = parse_uint(executor);
            break;

        case 'c':
            executor_cb.clear();
            break;

        case 'x':
            executor_cb.set_x(parse_uint(executor));
            break;

        case 'y':
            executor_cb.set_y(parse_uint(executor));
            break;

        default:
            break;
    }

    while (fetch_char(executor) != '}');
}

void tick_executor(executor_t* executor) {
    if (executor->sleep > current_time) {
        return;
    }

    while (current_time >= executor->prev_char + executor->period) {
        while (executor->active && peek_char(executor) == '{') {
            execute_instruction(executor);
            if (executor->sleep > current_time) {
                return;
            }
        }

        if (!executor->active) {
            return;
        }

        char c = fetch_char(executor);
        if (c == '\r' || c == '\n') {
            return;
        }

        if (c == '\\') {
            executor_cb.putchar('\n');
        } else {
            executor_cb.putchar(c);
        }

        executor->prev_char = current_time;
    };
}

void tick_executors() {
    for (uint32_t i = 0; i < executor_count; i++) {
        if (!executors[i].active) {
            continue;
        }
        tick_executor(&executors[i]);
    }

    current_time += 10;
}