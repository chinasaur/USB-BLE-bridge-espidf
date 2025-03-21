/*
 * Code to act as USB host for a CDC ACM compatible device.
 * Largely derived with modifications from the ESP-IDF cdc_acm_host example:
 *   https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/cdc/cdc_acm_host
 *
 * TODO(K6PLI): Debug why end of tx data corrupts beginning of next tx.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "usb/cdc_acm_host.h"

static const int USB_HOST_PRIORITY = 20;

void usb_setup();
bool usb_tx_blocking_if_connected(
    const uint8_t* buf, size_t buf_len, uint32_t timeout_ms);
void usb_register_new_data_receive_callback(cdc_acm_data_callback_t callback);
