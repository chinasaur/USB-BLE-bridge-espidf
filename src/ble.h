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

void setup_ble();

// TODO(K6PLI):
//   * Use uart_init stuff from spp_server example to make a write_to_ble call.
//   * Add register_ble_received_data_callback.
