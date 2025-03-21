#include "ble.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char* const TAG = "BRIDGE-BLE";
#define BLE_SVC_SPP_UUID16 (0xABF0)
#define BLE_SVC_SPP_CHR_UUID16 (0xABF1)
static const int CONFIG_EXAMPLE_IO_TYPE = 3;

static uint8_t address_type;
static bool client_notify_subscribed[CONFIG_BT_NIMBLE_MAX_CONNECTIONS + 1];
static ble_data_receive_callback_t ble_data_receive_callback = NULL;


void ble_register_new_data_receive_callback(
    ble_data_receive_callback_t callback) {
  ble_data_receive_callback = callback;
}

static uint16_t ble_spp_service_gatt_read_val_handle;
static int ble_service_gatt_handler(
    uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt* ctxt, void* arg);
static const struct ble_gatt_svc_def new_ble_service_gatt_defs[] = {
    {   // Service: SPP
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_SPP_UUID16),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {   // Single characteristic for READ | WRITE | NOTIFY.
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_SPP_CHR_UUID16),
                .access_cb = ble_service_gatt_handler,
                .val_handle = &ble_spp_service_gatt_read_val_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                         BLE_GATT_CHR_F_NOTIFY,
            }, { 0, },  // No more characteristics
        },
    }, { 0, },  // No more services.
};


static int ble_service_gatt_handler(
    uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt* ctxt, void* arg) {
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR:
    ESP_LOGI(TAG, "Callback for read");
    return 0;

  case BLE_GATT_ACCESS_OP_WRITE_CHR:
    ESP_LOGI(
        TAG,
        "Data received in write event; conn_handle = %x; attr_handle = %x; "
        "value = %.*s",
        conn_handle, attr_handle, ctxt->om->om_len, ctxt->om->om_data);
    if (!ble_data_receive_callback) return 0;
    return ble_data_receive_callback(ctxt->om->om_data, ctxt->om->om_len);

  default:
    ESP_LOGI(TAG, "Default Callback");
    return 0;
  }
}

static void ble_spp_server_on_reset(int reason) {
  ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void print_addr(const void *addr) {
  const uint8_t* u8p;
  u8p = addr;
  ESP_LOGI(TAG, "%02x:%02x:%02x:%02x:%02x:%02x",
           u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

// Log information about a connection to the console.
static void ble_spp_server_print_conn_desc(struct ble_gap_conn_desc *desc) {
    ESP_LOGI(TAG, "handle=%d our_ota_addr_type=%d our_ota_addr=",
             desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    ESP_LOGI(TAG, " our_id_addr_type=%d our_id_addr=",
             desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    ESP_LOGI(TAG, " peer_ota_addr_type=%d peer_ota_addr=",
             desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    ESP_LOGI(TAG, " peer_id_addr_type=%d peer_id_addr=",
             desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    ESP_LOGI(TAG,
             " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
             "encrypted=%d authenticated=%d bonded=%d\n",
             desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
             desc->sec_state.encrypted, desc->sec_state.authenticated,
             desc->sec_state.bonded);
}

static int ble_spp_server_gap_event(struct ble_gap_event *event, void *arg);
static void ble_spp_server_advertise() {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  const char* name;

  /**
   *  Set the advertisement data included in our advertisements:
   *     o Flags (indicates advertisement type and other general info).
   *     o Advertising tx power.
   *     o Device name.
   *     o 16-bit service UUIDs (alert notifications).
   */
  memset(&fields, 0, sizeof fields);

  /* Advertise two flags:
   *     o Discoverability in forthcoming advertisement (general)
   *     o BLE-only (BR/EDR unsupported).
   */
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  /* Indicate that the TX power level field should be included; have the
   * stack fill this value automatically.  This is done by assigning the
   * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
   */
  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  name = ble_svc_gap_device_name();
  fields.name = (uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  fields.uuids16 = (ble_uuid16_t[]) {
      BLE_UUID16_INIT(BLE_SVC_SPP_UUID16)
  };
  fields.num_uuids16 = 1;
  fields.uuids16_is_complete = 1;

  int rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error setting advertisement data; rc=%d\n", rc);
    return;
  }

  // Begin advertising.
  memset(&adv_params, 0, sizeof adv_params);
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  rc = ble_gap_adv_start(address_type, NULL, BLE_HS_FOREVER,
                         &adv_params, ble_spp_server_gap_event, NULL);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error enabling advertisement; rc=%d\n", rc);
    return;
  }
}

/**
 * The nimble host executes this callback when a GAP event occurs.
 *
 * @param event       The type of event being signalled.
 * @param ctxt        Various information pertaining to the event.
 * @param unused_arg  Application-specified argument; unused by ble_spp_server.
 *
 * @return            0 if the application successfully handled the
 *                    event; nonzero on failure.  The semantics
 *                    of the return code is specific to the
 *                    particular GAP event being signalled.
 */
static int ble_spp_server_gap_event(struct ble_gap_event *event, void* unused_arg) {
  struct ble_gap_conn_desc desc;
  int rc;
  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT:
    // A new connection was established or a connection attempt failed.
    ESP_LOGI(TAG, "Connection %s; status=%d ",
             event->connect.status == 0 ? "established" : "failed",
             event->connect.status);
    if (event->connect.status == 0) {
      rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
      assert(rc == 0);
      ble_spp_server_print_conn_desc(&desc);
    }
    if (event->connect.status != 0 || CONFIG_BT_NIMBLE_MAX_CONNECTIONS > 1) {
      // If connection failed or multiple connection allowed, resume advertising.
      ble_spp_server_advertise();
    }
    return 0;

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(TAG, "Disconnected; reason=%d ", event->disconnect.reason);
    ble_spp_server_print_conn_desc(&event->disconnect.conn);
    client_notify_subscribed[event->disconnect.conn.conn_handle] = false;
    ble_spp_server_advertise();  // Connection terminated; resume advertising.
    return 0;

  case BLE_GAP_EVENT_CONN_UPDATE:
    ESP_LOGI(  // The client has updated the connection parameters.
        TAG, "Connection updated; status=%d ", event->conn_update.status);
    rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
    assert(rc == 0);
    ble_spp_server_print_conn_desc(&desc);
    return 0;

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ESP_LOGI(
        TAG, "Advertising complete; reason=%d", event->adv_complete.reason);
    ble_spp_server_advertise();
    return 0;

  case BLE_GAP_EVENT_MTU:
    ESP_LOGI(TAG, "MTU update event; conn_handle=%d cid=%d mtu=%d\n",
             event->mtu.conn_handle, event->mtu.channel_id,
             event->mtu.value);
    return 0;

  case BLE_GAP_EVENT_SUBSCRIBE:
    ESP_LOGI(
        TAG,
        "Subscribe event; conn_handle=%d attr_handle=%d "
        "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
        event->subscribe.conn_handle, event->subscribe.attr_handle,
        event->subscribe.reason, event->subscribe.prev_notify,
        event->subscribe.cur_notify, event->subscribe.prev_indicate,
        event->subscribe.cur_indicate);
    client_notify_subscribed[
        event->subscribe.conn_handle] = event->subscribe.cur_notify;
    return 0;

  default:
    return 0;
  }
}

static void ble_spp_server_on_sync() {
  int rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  // Figure out address to use while advertising (no privacy for now).
  rc = ble_hs_id_infer_auto(0, &address_type);
  if (rc != 0) {
    ESP_LOGE(TAG, "Error determining address type; rc=%d\n", rc);
    return;
  }

  ESP_LOGI(TAG, "Address type: %d", address_type);
  uint8_t addr_val[6] = {0};
  rc = ble_hs_id_copy_addr(address_type, addr_val, NULL);
  assert(rc == 0);
  ESP_LOGI(TAG, "Device Address: ");
  print_addr(addr_val);

  ble_spp_server_advertise();
}

static void gatt_svr_register_cb(
    struct ble_gatt_register_ctxt *ctxt, void *arg) {
  char buf[BLE_UUID_STR_LEN];
  switch (ctxt->op) {
  case BLE_GATT_REGISTER_OP_SVC:
    ESP_LOGD(TAG, "Registered service %s with handle=%d\n",
             ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
    break;

  case BLE_GATT_REGISTER_OP_CHR:
    ESP_LOGD(TAG,
             "Registering characteristic %s with def_handle=%d val_handle=%d\n",
             ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
             ctxt->chr.def_handle, ctxt->chr.val_handle);
    break;

  case BLE_GATT_REGISTER_OP_DSC:
    ESP_LOGD(TAG, "Registering descriptor %s with handle=%d\n",
             ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
             ctxt->dsc.handle);
    break;

  default:
    assert(false);
    break;
  }
}

static int gatt_server_init(void) {
  ble_svc_gap_init();
  ble_svc_gatt_init();
  int rc = ble_gatts_count_cfg(new_ble_service_gatt_defs);
  if (rc != 0) return rc;
  rc = ble_gatts_add_svcs(new_ble_service_gatt_defs);
  if (rc != 0) return rc;
  return 0;
}

void ble_spp_server_host_task(void *param) {
  ESP_LOGI(TAG, "BLE Host Task Started");
  nimble_port_run();  // Returns only after nimble_port_stop() called.
  nimble_port_freertos_deinit();
}

void ble_store_config_init();
void ble_setup() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(nimble_port_init());
  for (int i = 0; i <= CONFIG_BT_NIMBLE_MAX_CONNECTIONS; ++i) {
    client_notify_subscribed[i] = false;
  }

  // Initialize the NimBLE host configuration.
  ble_hs_cfg.reset_cb = ble_spp_server_on_reset;
  ble_hs_cfg.sync_cb = ble_spp_server_on_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
  ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;

  // Register custom service.
  assert(gatt_server_init() == 0);
  assert(ble_svc_gap_device_name_set("USB-CDC-BLE-bridge") == 0);
  ble_store_config_init();
  nimble_port_freertos_init(ble_spp_server_host_task);
}

int ble_write_and_notify_subscribed_clients(const uint8_t* buf, size_t buf_len) {
  int clients_notified = 0;
  for (int i = 0; i <= CONFIG_BT_NIMBLE_MAX_CONNECTIONS; ++i) {
    if (!client_notify_subscribed[i]) continue;
    struct os_mbuf* txom = ble_hs_mbuf_from_flat(buf, buf_len);
    const int rc = ble_gatts_notify_custom(
        i, ble_spp_service_gatt_read_val_handle, txom);
    if (rc == 0) {
      ESP_LOGI(TAG, "Write and notify sent successfully; message: %.*s",
               buf_len, buf);
      ++clients_notified;
    } else {
      ESP_LOGI(TAG, "Error in write and notify; rc = %d", rc);
    }
  }
  return clients_notified;
}
