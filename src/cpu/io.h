#pragma once

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait() {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

static inline void mmio_write8(uintptr_t address, uint8_t value) {
    *(volatile uint8_t*) address = value;
}

static inline uint8_t mmio_read8(uintptr_t address) {
    return *(volatile uint8_t*) address;
}

static inline void mmio_write16(uintptr_t address, uint16_t value) {
    *(volatile uint16_t*) address = value;
}

static inline uint16_t mmio_read16(uintptr_t address) {
    return *(volatile uint16_t*) address;
}

static inline void mmio_write32(uintptr_t address, uint32_t value) {
    *(volatile uint32_t*) address = value;
}

static inline uint32_t mmio_read32(uintptr_t address) {
    return *(volatile uint32_t*) address;
}