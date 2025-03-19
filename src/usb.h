#pragma once

static const int USB_HOST_PRIORITY = 20;

void setup_usb();

// TODO(K6PLI):
//   * Use cdc_ach_host example to make a write_to_usb call.
//   * Add register_usb_received_data_callback.
