#include "pci.h"

#include <console/print.h>
#include <device/hda.h>
#include <cpu/io.h>

uint32_t pci_get_address(pci_id_t id, uint32_t offset) {
    return 0x80000000 | (id.bus << 16) | (id.slot << 11) | (id.func << 8) | (offset & 0xFC);
}

uint8_t pci_read8(pci_id_t id, uint32_t offset) {
    outl(0xCF8, pci_get_address(id, offset));
    return inb(0xCFC + (offset & 3));
}

void pci_write8(pci_id_t id, uint32_t offset, uint8_t value) {
    outl(0xCF8, pci_get_address(id, offset));
    outb(0xCFC + (offset & 3), value);
}

uint16_t pci_read16(pci_id_t id, uint32_t offset) {
    outl(0xCF8, pci_get_address(id, offset));
    return inw(0xCFC + (offset & 2));
}

void pci_write16(pci_id_t id, uint32_t offset, uint16_t value) {
    outl(0xCF8, pci_get_address(id, offset));
    outw(0xCFC + (offset & 2), value);
}

uint32_t pci_read32(pci_id_t id, uint32_t offset) {
    outl(0xCF8, pci_get_address(id, offset));
    return inl(0xCFC);
}

void pci_write32(pci_id_t id, uint32_t offset, uint32_t value) {
    outl(0xCF8, pci_get_address(id, offset));
    outl(0xCFC, value);
}

uint32_t pci_get_mmio_address(pci_id_t id, uint32_t bar) {
    return pci_read32(id, 0x10 + 4 * bar) & 0xFFFFFFF0;
}

uint16_t pci_get_port_offset(pci_id_t id, uint32_t bar) {
    return pci_read16(id, 0x10 + 4 * bar) & 0xFFFC;
}

void pci_check_function(pci_id_t id, uint16_t vendor_id) {
    pci_device_header_t header;
    header.vendor_id = vendor_id;
    header.device_id = pci_read16(id, 2);
    header.revision = pci_read8(id, 8);
    header.prog_if = pci_read8(id, 9);
    header.subclass = pci_read8(id, 10);
    header.class_code = pci_read8(id, 11);
    header.header_type = pci_read8(id, 14);

    if (header.class_code == 0xFF && header.subclass == 0xFF) {
        return;
    }

    kputs("pci: ");
    putc("0123456789ABCDEF"[header.class_code >> 4]);
    putc("0123456789ABCDEF"[header.class_code & 15]);
    putc(':');
    putc("0123456789ABCDEF"[header.subclass >> 4]);
    putc("0123456789ABCDEF"[header.subclass & 15]);
    putc('\n');

    if (header.class_code == 4 && header.subclass == 3) {
        puts("pci: found intel hda controller");
        hda_init(id, header);
    }
}

void pci_check_device(uint8_t bus, uint8_t device) {
    pci_id_t id;
    id.bus = bus;
    id.slot = device;
    id.func = 0;

    uint16_t vendor_id = pci_read16(id, 0);
    if (vendor_id == 0xFFFF) {
        return;
    }

    pci_check_function(id, vendor_id);

    if (pci_read8(id, 14) & 0x80) {
        for (id.func = 1; id.func < 8; id.func++) {
            uint16_t vendor_id = pci_read16(id, 0);
            if (vendor_id == 0xFFFF) {
                continue;
            }

            pci_check_function(id, vendor_id);
        }
    }
}

void pci_check_buses() {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check_device(bus, device);
        }
    }
}