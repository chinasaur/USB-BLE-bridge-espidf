#include "esp_log.h"
#include "usb.h"
#include "ble.h"

void app_main() {
  setup_usb();
  setup_ble();
}
