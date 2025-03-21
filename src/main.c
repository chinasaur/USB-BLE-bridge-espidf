/*
 * Simple program to bridge serial data communication from a USB CDC ACM device
 * to a BLE SPP service. The primary intent is to allow bluetooth access to
 * CAT control a radio.
 *
 * Tested on a ESP32-S3 DevKitC 2022-v1.3 development board (with dual USB-C
 * connectors). USB CAT connection tested with a QDX.  BLE client connection
 * tested with the nRF Connect app for Android.
 */

#include "esp_log.h"
#include "usb.h"
#include "ble.h"

int bridge_ble_data_to_usb(const uint8_t* data, size_t data_len) {
  usb_tx_blocking_if_connected(data, data_len, /*timeout_ms=*/1000);
  return 0;  // No error.
}

bool bridge_usb_data_to_ble(
    const uint8_t* data, size_t data_len, void* unused_arg) {
  ble_write_and_notify_subscribed_clients(data, data_len);
  return true;  // Data consumed.
}

void app_main() {
  usb_setup();
  ble_setup();
  usb_register_new_data_receive_callback(bridge_usb_data_to_ble);
  ble_register_new_data_receive_callback(bridge_ble_data_to_usb);
}
