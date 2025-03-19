/*
 * Simple program to bridge serial data communication from a USB CDC ACM device
 * to a BLE SPP service. The primary intent is to allow bluetooth access to
 * CAT control a radio.
 *
 * Tested on a ESP32-S3 DevKitC 2022-v1.3 development board (with dual USB-C
 * connectors). Tested connectivity to USB CAT device with a QDX. Tested
 * connectivity from BLE client Android phone running nRF Connect app.
 */

#include "esp_log.h"
#include "usb.h"
#include "ble.h"

void app_main() {
  setup_usb();
  setup_ble();
}
