#pragma once

#include <device/pci.h>

extern void* hda_data_buffer;
extern uint32_t hda_buffer_size;

void hda_alloc_data_buffer(uint32_t size);
void hda_init(pci_id_t id, pci_device_header_t header);
void hda_prepare();
void hda_start_transfer();
void hda_stop_playback();