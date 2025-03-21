# USB-BLE-bridge-espidf
A USB-BLE serial communication bridge for ESP32 boards with BLE and USB host
capability (e.g. ESP32-S3).

Currently this supports USB CDC ACM devices, and has been tested with a 
QRP Labs QDX transceiver. The USB CDC ACM host support is derived with
modifications from the espressif
[cdc_acm_host](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/usb/host/cdc/cdc_acm_host)
example.

The BLE serial interface is derived with modifications from the espressif
[spp_server](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/nimble/ble_spp/spp_server)
example, and has been tested with the nRF Connect app running on Android.

To support additional USB serial devices, a virtual COM port driver may need to
be integrated. There is an example VCP driver implementation in the
[espressif/esp-usb repo](https://github.com/espressif/esp-usb/tree/master/host/class/cdc/usb_host_vcp).

Currently disconnecting and reconnecting the QDX does not work on my hardware;
this requires QDX to be power cycled before it can be reconnected.
