/*
 * Code to set up a Bluetooth Low Energe (BLE) Serial Port Profile (SPP).
 * Largely derived with modifications from the ESP-IDF spp_server example:
 *   https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/nimble/ble_spp/spp_server
 *
 * Note that there is no official SPP standard for BLE. In this case a service
 * is created with a single READ / WRITE / NOTIFY characteristic that is used
 * for exchanging data.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

void ble_setup();
int ble_write_and_notify_subscribed_clients(const uint8_t* buf, size_t buf_len);

// TODO(K6PLI):
//   * Add register_ble_received_data_callback.
