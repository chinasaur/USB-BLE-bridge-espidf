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

typedef int (*ble_data_receive_callback_t)(
    const uint8_t* data, size_t data_len);

void ble_setup();
int ble_write_and_notify_subscribed_clients(const uint8_t* buf, size_t buf_len);
void ble_register_new_data_receive_callback(
    ble_data_receive_callback_t callback);
