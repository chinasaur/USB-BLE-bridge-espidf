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

void app_main() {
  usb_setup();
  /*ble_setup();*/

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(3000));

    // This basically works, although shows up on my phone as bytes instead of
    // text. Not sure how to write the characteristic as text from this end,
    // although I can do it from nRF Connect on the phone side.
    /*const uint8_t tx_str[] = "hello";*/
    /*ble_write_and_notify_subscribed_clients(tx_str, sizeof(tx_str));*/

    // This works the first time, getting two correct responses from QDX,
    // but after that the first ID; must be corrupted because the QDX sends
    // back "?;" followed by a single good response.
    //
    // TODO(phli(): Debug further. Trying to set the line coding, set line
    // state, send break, reset the endpoint, so far nothing has helped.
    const uint8_t tx_str[] = "ID;ID;";
    usb_tx_blocking_if_connected(tx_str, sizeof(tx_str), 1000);

  }
}
