/*
 * Code to act as USB host for a CDC ACM compatible device.
 * Largely derived with modifications from the ESP-IDF cdc_acm_host example:
 *   https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/cdc/cdc_acm_host
 */
#pragma once

static const int USB_HOST_PRIORITY = 20;

void setup_usb();

// TODO(K6PLI):
//   * Use cdc_ach_host example to make a write_to_usb call.
//   * Add register_usb_received_data_callback.
