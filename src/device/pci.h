#pragma once

#include <stdint.h>

typedef struct {
    uint32_t bus;
    uint32_t slot;
    uint32_t func;
} pci_id_t;

typedef struct pci_device_header {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
    uint8_t header_type;
} pci_device_header_t;

uint8_t pci_read8(pci_id_t id, uint32_t offset);
void pci_write8(pci_id_t id, uint32_t offset, uint8_t value);

uint16_t pci_read16(pci_id_t id, uint32_t offset);
void pci_write16(pci_id_t id, uint32_t offset, uint16_t value);

uint32_t pci_read32(pci_id_t id, uint32_t offset);
void pci_write32(pci_id_t id, uint32_t offset, uint32_t value);

uint32_t pci_get_mmio_address(pci_id_t id, uint32_t bar);
uint16_t pci_get_port_offset(pci_id_t id, uint32_t bar);

void pci_check_buses();