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
#include "freertos/ringbuf.h"
#include "ble.h"
#include "usb.h"

#define BUFFER_SIZE (4096)

int bridge_ble_data_to_usb(const uint8_t* data, size_t data_len) {
  usb_tx_blocking_if_connected(data, data_len, /*timeout_ms=*/1000);
  return 0;  // No error.
}

// Simply copies data from USB to BLE as it comes in.
bool bridge_usb_data_to_ble_direct(
    const uint8_t* data, size_t data_len, void* unused_arg) {
  ble_write_and_notify_subscribed_clients(data, data_len);
  return true;  // Data consumed.
}

// Buffers incoming USB data until the end of a message (a semicolon) is 
// detected, and then sends the complete message to BLE.
bool bridge_usb_data_to_ble_buffer_to_end_of_message(
    const uint8_t* data, size_t data_len, void* unused_arg) {
  static uint8_t buffer[BUFFER_SIZE];
  static int buffer_index = 0;
  for (int i = 0; i < data_len; ++i) {
    buffer[buffer_index++] = data[i];
    if (data[i] == ';' || buffer_index == BUFFER_SIZE) {
      ble_write_and_notify_subscribed_clients(buffer, buffer_index);
      buffer_index = 0;
    }
  }
  return true;  // Data consumed.
}

void app_main() {
  ble_setup();
  usb_setup();

  ble_register_new_data_receive_callback(bridge_ble_data_to_usb);

  // Pick either one of these callbacks.  Buffering is a bit nicer for human
  // readability but otherwise either should be fine.
  // usb_register_new_data_receive_callback(bridge_usb_data_to_ble_direct)
  usb_register_new_data_receive_callback(
      bridge_usb_data_to_ble_buffer_to_end_of_message);
}
