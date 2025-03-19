#include "usb.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "usb/cdc_acm_host.h"

static const char* const TAG = "BRIDGE-USB";

static void usb_lib_task(void* unused_arg) {
  while (true) {
    uint32_t event_flags;
    ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      ESP_ERROR_CHECK(usb_host_device_free_all());
    }
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
      ESP_LOGI(TAG, "All devices freed");
    }
  }
}

static void handle_cdc_event(const cdc_acm_host_dev_event_data_t* event, void* user_ctx) {
  switch (event->type) {
  case CDC_ACM_HOST_ERROR:
    ESP_LOGE(TAG, "CDC-ACM error has occurred, err_no = %i", event->data.error);
    break;
  case CDC_ACM_HOST_DEVICE_DISCONNECTED:
    ESP_LOGI(TAG, "Device disconnected");
    ESP_ERROR_CHECK(cdc_acm_host_close(event->data.cdc_hdl));
    break;
  case CDC_ACM_HOST_SERIAL_STATE:
    ESP_LOGI(TAG, "Serial state notif 0x%04X", event->data.serial_state.val);
    break;
  case CDC_ACM_HOST_NETWORK_CONNECTION:
  default:
    ESP_LOGW(TAG, "Unsupported CDC event: %i", event->type);
    break;
  }
}

static bool handle_cdc_rx(const uint8_t *data, size_t data_len, void* unused_arg) {
  ESP_LOGI(TAG, "Data received: %.*s", data_len, data);
  // TODO(K6PLI): Add a callback here to bridge data to BLE.
  return true;
}

static SemaphoreHandle_t new_device_semaphore;

static void handle_new_device(usb_device_handle_t new_usb_device) {
  assert(new_device_semaphore);
  xSemaphoreGive(new_device_semaphore);
}

static void new_device_task(void* unused_arg) {
  const cdc_acm_host_device_config_t cdc_device_config = {
      .connection_timeout_ms = 1000,
      .out_buffer_size = 512,
      .in_buffer_size = 512,
      .user_arg = NULL,
      .event_cb = handle_cdc_event,
      .data_cb = handle_cdc_rx,
  };
  while(true) {
    assert(new_device_semaphore);
    xSemaphoreTake(new_device_semaphore, portMAX_DELAY);
    cdc_acm_dev_hdl_t cdc_device = NULL;
    ESP_LOGI(TAG, "Opening CDC ACM device...");
    esp_err_t err = cdc_acm_host_open(
        CDC_HOST_ANY_VID, CDC_HOST_ANY_PID, 0, &cdc_device_config, &cdc_device);
    if (err != ESP_OK) {
      ESP_LOGI(TAG, "Failed to open device as CDC ACM.");
      continue;
    }
    cdc_acm_host_desc_print(cdc_device);
  }
}

void setup_usb() {
  ESP_LOGI(TAG, "Installing USB Host");
  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  BaseType_t task_created = xTaskCreate(
      usb_lib_task, "usb_lib", 4096, xTaskGetCurrentTaskHandle(), USB_HOST_PRIORITY, NULL);
  assert(task_created == pdTRUE);

  ESP_LOGI(TAG, "Installing CDC-ACM driver");
  ESP_ERROR_CHECK(cdc_acm_host_install(NULL));

  new_device_semaphore = xSemaphoreCreateBinary();
  ESP_ERROR_CHECK(cdc_acm_host_register_new_dev_callback(handle_new_device));
  task_created = xTaskCreate(
      new_device_task, "new_device", 4096, xTaskGetCurrentTaskHandle(), USB_HOST_PRIORITY, NULL);
  assert(task_created == pdTRUE);
}
