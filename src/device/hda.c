#include "hda.h"

#include <cpu/io.h>
#include <console/print.h>
#include <device/pit.h>
#include <util/lib.h>

#define VOLUME 50

typedef struct {
    uint32_t* memory;
    uint32_t pointer;
    uint32_t entries;
} hda_ringbuffer_t;

typedef struct {
    uint8_t operational;
    uint32_t mmio_base;
    uint32_t output_stream;
    uint32_t* output_buffers;
    hda_ringbuffer_t corb;
    hda_ringbuffer_t rirb;
    uint8_t pio;
    uint32_t codec;
    uint32_t afg_node;
    uint32_t* output_nodes;
    uint32_t num_output_nodes;
} hda_device_t;

static hda_device_t* devices = 0;
static uint32_t device_count = 0;

void* hda_data_buffer;
uint32_t hda_buffer_size;

static hda_device_t* allocate_device() {
    if (!devices) {
        devices = calloc(sizeof(hda_device_t) * 8);
    }

    hda_device_t* device = &devices[device_count++];

    return device;
}

uint8_t hda_mmio_wait32(hda_device_t* device, uint32_t max_ticks, uint32_t offset, uint32_t mask, uint32_t value) {
    pit_ticks = 0;
    while (pit_ticks < max_ticks) {
        if ((mmio_read32(device->mmio_base + offset) & mask) == value) {
            return 1;
        }
    }
    return 0;
}

uint8_t hda_mmio_wait16(hda_device_t* device, uint32_t max_ticks, uint32_t offset, uint16_t mask, uint16_t value) {
    pit_ticks = 0;
    while (pit_ticks < max_ticks) {
        if ((mmio_read16(device->mmio_base + offset) & mask) == value) {
            return 1;
        }
    }
    return 0;
}

uint8_t hda_mmio_wait8(hda_device_t* device, uint32_t max_ticks, uint32_t offset, uint8_t mask, uint8_t value) {
    pit_ticks = 0;
    while (pit_ticks < max_ticks) {
        if ((mmio_read8(device->mmio_base + offset) & mask) == value) {
            return 1;
        }
    }
    return 0;
}

void hda_advance_ring_buffer(hda_ringbuffer_t* buffer) {
    buffer->pointer++;
    if (buffer->pointer >= buffer->entries) {
        buffer->pointer = 0;
    }
}

uint32_t hda_send_verb(hda_device_t* device, uint32_t codec, uint32_t node, uint32_t verb, uint32_t command) {
    uint32_t payload = (codec << 28) | (node << 20) | (verb << 8) | command;

    if (device->pio) {
        mmio_write16(device->mmio_base + 0x68, 2);
        mmio_write32(device->mmio_base + 0x60, payload);
        mmio_write16(device->mmio_base + 0x68, 1);

        if (!hda_mmio_wait16(device, 2, 0x68, 3, 2)) {
            puts("hda: no response (pio)");
            return 0;
        }

        mmio_write16(device->mmio_base + 0x68, 2);
        return mmio_read32(device->mmio_base + 0x64);
    }

    device->corb.memory[device->corb.pointer] = payload;
    mmio_write16(device->mmio_base + 0x48, device->corb.pointer);

    if (!hda_mmio_wait16(device, 2, 0x58, 0xFFFF, device->corb.pointer)) {
        puts("hda: no response (corb/rirb)");
        return 0;
    }

    payload = device->rirb.memory[device->rirb.pointer * 2];

    hda_advance_ring_buffer(&device->corb);
    hda_advance_ring_buffer(&device->rirb);

    return payload;
}

uint32_t hda_get_node_type(hda_device_t* device, uint32_t node) {
    return (hda_send_verb(device, device->codec, node, 0xF00, 9) >> 20) & 0xF;
}

void hda_set_node_volume(hda_device_t* device, uint32_t node, uint32_t cap) {
    uint32_t payload = 0x3000 | 0x8000;

    payload |= ((cap >> 8) & 0x7F) * VOLUME / 100;
    hda_send_verb(device, device->codec, node, 0x300, payload);
}

uint32_t hda_get_node_connection(hda_device_t* device, uint32_t codec, uint32_t node, uint32_t number) {
    uint32_t connection_cap = hda_send_verb(device, codec, node, 0xF00, 0xE);
    if (connection_cap >= (connection_cap & 0x7F)) {
        return 0;
    }

    if (connection_cap & 0x80) {
        return (hda_send_verb(device, codec, node, 0xF02, ((number / 2) * 2)) >> ((number % 2) * 16)) & 0xFFFF;
    } else {
        return (hda_send_verb(device, codec, node, 0xF02, ((number / 4) * 4)) >> ((number % 4) * 8)) & 0xFF;
    }
}

void hda_init_output_node(hda_device_t* device, uint32_t node) {
    for (uint32_t i = 0; i < device->num_output_nodes; i++) {
        if (device->output_nodes[i] == node) {
            return;
        }
    }

    kputs("hda: found output node ");

    hda_send_verb(device, device->codec, node, 0x705, 0);
    hda_send_verb(device, device->codec, node, 0x708, 0);
    hda_send_verb(device, device->codec, node, 0x703, 0);

    uint32_t amp_cap = hda_send_verb(device, device->codec, node, 0xF00, 0x12);
    hda_set_node_volume(device, node, amp_cap);

    uint32_t compatible = 1;

    kputs("[ ");

    uint32_t stream_cap = hda_send_verb(device, device->codec, node, 0xF00, 0x0B);
    if (!stream_cap) {
        stream_cap = hda_send_verb(device, device->codec, device->afg_node, 0xF00, 0x0B);
    }

    if (stream_cap & 1) {
        kputs("pcm ");
    } else {
        compatible = 0;
    }

    uint32_t sample_cap = hda_send_verb(device, device->codec, node, 0xF00, 0x0A);
    if (!sample_cap) {
        sample_cap = hda_send_verb(device, device->codec, device->afg_node, 0xF00, 0x0A);
    }

    const char* sample_rates[] = {"8000", "11025", "16000", "22050", "32000", "44100",
                                  "48000", "88200", "96000", "176400", "192000"};

    for (int i = 0; i < 11; i++) {
        if (sample_cap & (1 << i)) {
            kputs(sample_rates[i]);
            putc(' ');
        }
    }

    if (!(sample_cap & (1 << 5))) {
        compatible = 0;
    }

    kputs("] ");

    if (compatible) {
        kputs("(COMPATIBLE) ");
    }

    putc('\n');

    if (compatible) {
        hda_send_verb(device, device->codec, node, 0x706, 0x10);
        device->output_nodes[device->num_output_nodes++] = node;
        device->operational |= 1;
    }
}

void hda_init_complex_node(hda_device_t* device, uint32_t node, uint32_t path_length) {
    if (path_length > 10) {
        return;
    }

    hda_send_verb(device, device->codec, node, 0x705, 0);
    hda_send_verb(device, device->codec, node, 0x708, 0);
    hda_send_verb(device, device->codec, node, 0x703, 0);
    hda_send_verb(device, device->codec, node, 0x707, hda_send_verb(device, device->codec, node, 0xF07, 0) | 0x40 | 0x80);
    hda_send_verb(device, device->codec, node, 0x70C, 6);

    uint32_t output_amp_cap = hda_send_verb(device, device->codec, node, 0xF00, 0x12);
    hda_set_node_volume(device, node, output_amp_cap);

    uint32_t number = 0;
    uint32_t connection = hda_get_node_connection(device, device->codec, node, 0);
    while (connection) {
        uint32_t type = hda_get_node_type(device, connection);
        if (type == 0) {
            hda_init_output_node(device, connection);
        } else {
            hda_init_complex_node(device, connection, path_length + 1);
        }
        connection = hda_get_node_connection(device, device->codec, node, ++number);
    }
}

void hda_init_node(hda_device_t* device, uint32_t node) {
    uint32_t node_type = hda_get_node_type(device, node);
    if (node_type == 4) {
        hda_init_complex_node(device, node, 0);
    } else {
        hda_init_output_node(device, node);
    }
}

uint8_t hda_init_afg(hda_device_t* device, uint32_t afg_node) {
    hda_send_verb(device, device->codec, afg_node, 0x7FF, 0);
    hda_send_verb(device, device->codec, afg_node, 0x705, 0);
    hda_send_verb(device, device->codec, afg_node, 0x708, 0);

    device->afg_node = afg_node;

    uint32_t node_count = hda_send_verb(device, device->codec, device->afg_node, 0xF00, 4);
    uint32_t first_node = (node_count >> 16) & 0xFF;
    uint32_t last_node = first_node + (node_count & 0xFF);
    for (uint32_t node = first_node; node < last_node; node++) {
        hda_init_node(device, node);
    }

    return 1;
}

uint8_t hda_init_codec(hda_device_t* device, uint32_t codec) {
    device->codec = codec;

    uint32_t node_count = hda_send_verb(device, device->codec, 0, 0xF00, 4);
    uint32_t first_node = (node_count >> 16) & 0xFF;
    uint32_t last_node = first_node + (node_count & 0xFF);
    for (uint32_t node = first_node; node < last_node; node++) {
        if ((hda_send_verb(device, device->codec, node, 0xF00, 5) & 0x7F) == 1) {
            puts("hda: found audio function group");
            if (hda_init_afg(device, node)) {
                return 1;
            }
        }
    }

    puts("hda: couldn't find audio function group");
    return 0;
}

uint32_t hda_get_ringbuffer_entry_count(hda_device_t* device, uint32_t offset) {
    uint8_t data = mmio_read8(device->mmio_base + offset);
    if (data & 0x40) {
        mmio_write8(device->mmio_base + offset, 2);
        return 256;
    } else if (data & 0x20) {
        mmio_write8(device->mmio_base + offset, 1);
        return 16;
    } else if (data & 0x10) {
        mmio_write8(device->mmio_base + offset, 0);
        return 2;
    }

    return 0;
}

uint8_t hda_init_corb(hda_device_t* device) {
    device->corb.pointer = 1;
    device->corb.memory = calloca(1024, 128);

    mmio_write32(device->mmio_base + 0x40, (uintptr_t) device->corb.memory);
    mmio_write32(device->mmio_base + 0x44, 0);

    device->corb.entries = hda_get_ringbuffer_entry_count(device, 0x4E);
    if (device->corb.entries == 0) {
        return 0;
    }

    mmio_write16(device->mmio_base + 0x4A, 0x8000);
    if (!hda_mmio_wait16(device, 5, 0x4A, 0x8000, 0x8000)) {
        return 0;
    }

    mmio_write16(device->mmio_base + 0x4A, 0);
    if (!hda_mmio_wait16(device, 5, 0x4A, 0x8000, 0)) {
        return 0;
    }

    mmio_write16(device->mmio_base + 0x48, 0);

    return 1;
}

uint8_t hda_init_rirb(hda_device_t* device) {
    device->rirb.pointer = 1;
    device->rirb.memory = calloca(2048, 128);

    mmio_write32(device->mmio_base + 0x50, (uintptr_t) device->rirb.memory);
    mmio_write32(device->mmio_base + 0x54, 0);

    device->rirb.entries = hda_get_ringbuffer_entry_count(device, 0x5E);
    if (device->rirb.entries == 0) {
        return 0;
    }

    mmio_write16(device->mmio_base + 0x58, 0x8000);

    pit_ticks = 0;
    while (pit_ticks < 5);

    mmio_write16(device->mmio_base + 0x5A, 0);

    return 1;
}

void hda_disable_corb_rirb(hda_device_t* device) {
    mmio_write32(device->mmio_base + 0x4C, 0);
    mmio_write32(device->mmio_base + 0x5C, 0);
}

void hda_enable_corb_rirb(hda_device_t* device) {
    mmio_write8(device->mmio_base + 0x4C, 2);
    mmio_write8(device->mmio_base + 0x5C, 2);
}

void hda_init(pci_id_t id, pci_device_header_t header) {
    puts("hda: found new device");

    hda_device_t* device = allocate_device();
    device->mmio_base = pci_get_mmio_address(id, 0);

    pci_write8(id, 0x04, pci_read8(id, 0x04) | 6);

    mmio_write32(device->mmio_base + 8, 0);
    hda_mmio_wait32(device, 5, 8, 1, 0);
    mmio_write32(device->mmio_base + 8, 1);
    if (!hda_mmio_wait32(device, 5, 8, 1, 1)) {
        puts("hda: failed to reset the card");
        return;
    }

    mmio_write32(device->mmio_base + 0x20, 0);
    mmio_write32(device->mmio_base + 0x70, 0);
    mmio_write32(device->mmio_base + 0x74, 0);
    mmio_write32(device->mmio_base + 0x34, 0);
    mmio_write32(device->mmio_base + 0x38, 0);

    hda_disable_corb_rirb(device);

    if (!hda_init_corb(device)) {
        puts("hda: failed to init corb");
        device->pio |= 1;
    }

    if (!hda_init_rirb(device)) {
        puts("hda: failed to init rirb");
        device->pio |= 1;
    }

    device->output_nodes = calloc(256);
    device->output_stream = device->mmio_base + 0x80 + 0x20 * ((mmio_read16(device->mmio_base) >> 8) & 0xF);
    device->output_buffers = calloca(32, 128);

    search_codec:
    if (device->pio) {
        hda_disable_corb_rirb(device);
    } else {
        hda_enable_corb_rirb(device);
    }

    for (uint32_t codec = 0; codec < 16; codec++) {
        uint32_t codec_id = hda_send_verb(device, codec, 0, 0xF00, 0);

        if (codec_id) {
            puts("hda: found codec");
            if (hda_init_codec(device, codec)) {
                return;
            }
        }
    }

    if (!device->pio) {
        puts("hda: couldn't find codec with corb/rirb, trying again with pio...");
        device->pio |= 1;
        goto search_codec;
    } else {
        puts("hda: couldn't find codec");
    }
}

void hda_alloc_data_buffer(uint32_t size) {
    hda_data_buffer = calloca(size, 128);
    hda_buffer_size = size;
}

void hda_prepare_device_play(hda_device_t* device) {
    mmio_write8(device->output_stream, 0);
    if (!hda_mmio_wait8(device, 2, device->output_stream - device->mmio_base, 2, 0)) {
        puts("hda: failed to stop stream");
        return;
    }

    mmio_write8(device->output_stream, 1);
    if (!hda_mmio_wait8(device, 5, device->output_stream - device->mmio_base, 1, 1)) {
        puts("hda: failed to reset stream");
        return;
    }

    pit_ticks = 0;
    while (pit_ticks < 2);

    mmio_write8(device->output_stream, 0);
    if (!hda_mmio_wait8(device, 5, device->output_stream - device->mmio_base, 1, 0)) {
        puts("hda: failed to reset stream");
        return;
    }

    pit_ticks = 0;
    while (pit_ticks < 2);

    mmio_write8(device->output_stream + 3, 0x1C);

    memset(device->output_buffers, 0, 32);
    device->output_buffers[0] = (uintptr_t) hda_data_buffer;
    device->output_buffers[2] = hda_buffer_size;
    asm volatile("wbinvd");

    mmio_write32(device->output_stream + 0x18, (uintptr_t) device->output_buffers);
    mmio_write32(device->output_stream + 0x08, hda_buffer_size);
    mmio_write32(device->output_stream + 0x0C, 1);

    uint32_t format = (1 << 4) | (0b1000000 << 8);
    mmio_write16(device->output_stream + 0x12, format);

    for (uint32_t i = 0; i < device->num_output_nodes; i++) {
        hda_send_verb(device, device->codec, device->output_nodes[i], 0x200, format);
    }

    pit_ticks = 0;
    while (pit_ticks < 2);
}

void hda_prepare() {
    for (uint32_t i = 0; i < device_count; i++) {
        if (!devices[i].operational) {
            continue;
        }
        hda_prepare_device_play(&devices[i]);
    }
}

void hda_start_transfer() {
    for (uint32_t i = 0; i < device_count; i++) {
        if (!devices[i].operational) {
            continue;
        }
        mmio_write8(devices[i].output_stream + 2, 0x14);
        mmio_write8(devices[i].output_stream, 2);
    }
}

void hda_stop_playback() {
    for (uint32_t i = 0; i < device_count; i++) {
        if (!devices[i].operational) {
            continue;
        }
        mmio_write8(devices[i].output_stream, 0);
    }
}