/*
 * Code to act as USB host for a CDC ACM compatible device.
 * Largely derived with modifications from the ESP-IDF cdc_acm_host example:
 *   https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/cdc/cdc_acm_host
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const int USB_HOST_PRIORITY = 20;

void usb_setup();
bool usb_tx_blocking_if_connected(
    const uint8_t* buf, size_t buf_len, uint32_t timeout_ms);


// Temporary hacking.
#include "usb/cdc_acm_host.h"
cdc_acm_dev_hdl_t usb_get_device();


// TODO(K6PLI):
//   * Add register_usb_received_data_callback.
//   * Debug why only one CDC message gets through correctly.
