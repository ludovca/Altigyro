#include <stdio.h>
#include <string.h>
#include <math.h>

#include "nvs_flash.h"
#include "esp_log.h"

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_hs_adv.h"

#define MAX_DEVICES 50

typedef struct {
    ble_addr_t addr;
    int rssi;
    float distance;
    uint32_t last_seen; 
    char name[32];
    char brand[32];
    char type[32];
    char model[32];
    char addr_str[18];      // cached address string
    char last_line[512];    // last printed UI line
    uint32_t apple_subtypes_seen; 
    bool used;
    uint16_t company_id;
    uint8_t first_subtype;
    uint8_t adv_len;
    int8_t last_rssi;
} device_entry_t;

typedef struct {
    uint16_t id;
    const char *brand;
} company_id_t;

static const company_id_t known_companies[] = {
    {0x004C, "Apple / Beats"},
    {0x0133, "Beats Electronics"},
    {0x0075, "Samsung"},
    {0x000F, "Broadcom"},
    {0x0006, "Microsoft"},
    {0x0059, "Nordic Semiconductor"},
    {0x00E0, "Google"},
    {0xFFFF, "Generic Chinese Vendor"},
    {0x007D, "Sony"},                   // WH-1000XM headphones, PS controllers
    {0x00D2, "Bose"},                   // QuietComfort, SoundLink
    {0x00A0, "Garmin"},                 // Watches, bike sensors
    {0x00C7, "Huawei"},                 // Phones, earbuds, wearables
    {0x0157, "Xiaomi"},                 // Mi Band, scooters, smart devices
    {0x02E5, "OnePlus"},                // Earbuds, phones
    {0x0424, "Anker / Soundcore"},      // Soundcore earbuds & speakers
    {0x0171, "JBL / Harman"},           // JBL speakers & headphones
    {0x00E3, "Amazon"},                 // Echo devices, trackers
    {0x00D6, "Logitech"},               // Keyboards, mice, gamepads
    {0x00E5, "Razer"},                  // Headsets, controllers
    {0x00F0, "Fitbit (old)"},           // Older Fitbit models
    {0x01DA, "Tile"},                   // Tile trackers
    {0x02E0, "Wyze"},                   // Smart home sensors
    {0x0047, "HTC"},
    {0x0048, "Qualcomm"},
    {0x004F, "Motorola"},
    {0x005A, "Dialog Semiconductor"},
    {0x0065, "Plantronics / Poly"},
    {0x0098, "Withings"},
    {0x00A6, "Polar"},                  // Heart rate sensors
    {0x00C8, "Suunto"},                 // Sports watches
    {0x00D0, "HP"},
    {0x00E1, "Nintendo"},               // Joy‑Cons, Pro Controller
    {0x012D, "Under Armour"},           // UA Record sensors
    {0x0147, "Philips"},                // Hue, toothbrushes
    {0x0150, "Sennheiser"},             // Headphones
    {0x0160, "GoPro"},
    {0x01A1, "DJI"},                    // Drones, controllers
}; 

static device_entry_t devices[MAX_DEVICES];
static const char *TAG = "BLE_SCAN";

/* ------------------ Helpers ------------------ */
bool is_airpods(const uint8_t *md, int len){

    if (len < 5) return false;
    if (md[0]!=0x4C || md[1]!=0x00) return false;
    if(md[2]==0x07 && md[3] == 0x19) return true;
    if(md[2] == 0x06 && md[3] == 0x03) return true;
    if(len>= 25 && md[2] == 0x01) return true;

    return false;
}

static void decide_device_type(device_entry_t *dev,
                               const char *sp_type,
                               const char *apple_type,
                               uint8_t apple_subtype,
                               const char *name)
{
    // Track all Apple subtypes seen for this device
    if (apple_subtype != 0xFF) {
        dev->apple_subtypes_seen |= (1u << (apple_subtype & 0x1F)); // 0–31
    }

    // 1) Name-based Beats override (strongest signal)
    if (name && (strstr(name, "Beats") || strstr(name, "Studio"))) {
        strncpy(dev->type, "Beats Headphones", sizeof(dev->type) - 1);
        dev->type[sizeof(dev->type) - 1] = '\0';
        return;
    }

    // 3) Normal priority: Swift Pair > Apple type
    if (sp_type) {
        strncpy(dev->type, sp_type, sizeof(dev->type) - 1);
        dev->type[sizeof(dev->type) - 1] = '\0';
    } else if (apple_type) {
        strncpy(dev->type, apple_type, sizeof(dev->type) - 1);
        dev->type[sizeof(dev->type) - 1] = '\0';
    }
}

static const char* get_swift_pair_type(const uint8_t *mfg, uint8_t len)
{
    // Need at least: company(2) + beacon(1) + type(1)
    if (len < 4) return NULL;

    uint16_t company_id = mfg[0] | (mfg[1] << 8);
    uint8_t beacon_id   = mfg[2];
    uint8_t device_type = mfg[3];

    if (company_id != 0x0006 || beacon_id != 0x01) {
        return NULL; // Not a Microsoft Swift Pair packet
    }

    switch (device_type) {
        case 0x01: return "Mouse";
        case 0x02: return "Keyboard";
        case 0x03: return "Number Pad";
        case 0x04: return "Pen";
        case 0x05: return "Audio";
        case 0x06: return "Controller";
        case 0x07: return "PC";
        default:   return "Microsoft Device";
    }
}

static const char* get_apple_type(const uint8_t *mfg, uint8_t len)
{
    if (len < 3) return NULL;

    uint16_t company_id = mfg[0] | (mfg[1] << 8);
    if (company_id != 0x004C) return NULL; // Not Apple

    uint8_t subtype = mfg[2];

    switch (subtype) {
        case 0x06: return "iPhone / iPad / Mac";
        case 0x07: return "Apple Watch";
        case 0x0A: return "AirPods";
        case 0x0C: return "AirTag / Find My";
        case 0x0F: return "HomePod";
        case 0x10: return "Beats(not)";
        case 0x12: return "Apple TV";
        case 0x1F: return "AirPods Pro 2";
        default:   return "Apple Device";
    }
}

static float estimate_distance(int rssi, int tx_power)
{
    // Path-loss exponent (environment factor)
    // 2.0 = free space, 2.7–3.5 = indoors, 4.0+ = obstructed
    const float n = 2.2f;

    // Formula: d = 10 ^ ((TxPower - RSSI) / (10 * n))
    float ratio = (tx_power - rssi) / (10.0f * n);
    return powf(10.0f, ratio);
}

static void addr_to_str(const ble_addr_t *addr, char *str, size_t size)
{
    snprintf(str, size,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             addr->val[5], addr->val[4], addr->val[3],
             addr->val[2], addr->val[1], addr->val[0]);
}

static const char* get_brand_from_mfg(const uint8_t *data, uint8_t len)
{
    if (len < 2) return "Unknown";

    uint16_t company_id = data[0] | (data[1] << 8);

    for (int i = 0; i < sizeof(known_companies)/sizeof(known_companies[0]); i++) {
        if (known_companies[i].id == company_id) {
            return known_companies[i].brand;
        }
    }
    return "Unknown";
}

/* ------------------ UI ------------------ */

static void screen_clear(void)
{
    printf("\033[2J");  // clear screen
    printf("\033[H");   // move cursor to top-left
}

static void print_table(void)
{
    static bool header_drawn = false;

    if (!header_drawn) {
        screen_clear();
        printf("ADDR                RSSI   DIST(m)   TYPE           NAME                     BRAND           MODEL\n");
        printf("-------------------------------------------------------------------------------------------\n");
        header_drawn = true;
    }

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!devices[i].used) continue;

        char line[512];
        snprintf(line, sizeof(line),
                 "%-18s  %-5d  %-8.2f  %-13s  %-24s  %-15s  %s",
                 devices[i].addr_str,
                 devices[i].rssi,
                 devices[i].distance,
                 devices[i].type[0] ? devices[i].type : "-",
                 devices[i].name[0] ? devices[i].name : "(none)",
                 devices[i].brand[0] ? devices[i].brand : "Unknown",
                 devices[i].model[0] ? devices[i].model : "-");

        if (strcmp(line, devices[i].last_line) != 0) {
            printf("\033[%d;1H%s", i + 3, line);
            strncpy(devices[i].last_line, line, sizeof(devices[i].last_line));
        }
    }
}

static void cleanup_task(void *arg)
{
    const uint32_t TIMEOUT = 5000 / portTICK_PERIOD_MS;  // 5 seconds

    while (1) {
        uint32_t now = xTaskGetTickCount();

        for (int i = 0; i < MAX_DEVICES; i++) {
            if (devices[i].used &&
                (now - devices[i].last_seen) > TIMEOUT) {

                devices[i].used = false;  // remove device
            }
        }

        print_table();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
/* ------------------ BLE Scan Callback ------------------ */

static int on_model_read(uint16_t conn_handle,
                         const struct ble_gatt_error *error,
                         struct ble_gatt_attr *attr,
                         void *arg)
{
    if (error->status != 0) return 0;

    int index = (int)(intptr_t)arg;   // device index passed from connect
    if (index >= 0 && index < MAX_DEVICES) {
        snprintf(devices[index].model,
                 sizeof(devices[index].model),
                 "%.*s",
                 attr->om->om_len,
                 (char *)attr->om->om_data);
    }

    print_table();  // ⭐ update UI

    return 0;
}

static int on_characteristic(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             const struct ble_gatt_chr *chr,
                             void *arg)
{
    if (error->status != 0) return 0;

    if (chr->uuid.u16.value == 0x2A24) { // Model Number
        printf("Found Model Number characteristic\n");
        ble_gattc_read(conn_handle, chr->val_handle, on_model_read, arg);   // pass index forward

    }
    return 0;
}

static int on_service(uint16_t conn_handle,
                      const struct ble_gatt_error *error,
                      const struct ble_gatt_svc *svc,
                      void *arg)
{
    if (error->status != 0) return 0;

    if (svc->uuid.u16.value == 0x180A) { // Device Information Service
        printf("Found Device Information Service\n");
        ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle, on_characteristic, arg);   // pass index forward

    }
    return 0;
}

static int on_connect(struct ble_gap_event *event, void *arg)
{
    if (event->connect.status == 0) {
        printf("Connected, discovering services...\n");
       ble_gattc_disc_all_svcs(event->connect.conn_handle, on_service, arg);   // pass index forward
    }
    return 0;
}

static void connect_to_device(const ble_addr_t *addr, int index)
{
    ble_gap_connect(BLE_OWN_ADDR_PUBLIC, addr, 30000, NULL,
                    on_connect, (void *)(intptr_t)index);
}


static int scan_cb(struct ble_gap_event *event, void *arg)
{
    if (event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    const ble_addr_t *addr = &event->disc.addr;
    int rssi = event->disc.rssi;

    /* Calculate distance */
    float distance = estimate_distance(rssi, -59);

    /* Parse advertisement fields */
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    ble_hs_adv_parse_fields(&fields,
                            event->disc.data,
                            event->disc.length_data);

    /* Manufacturer data */
    const uint8_t *md = NULL;
    int md_len = 0;

    if (fields.mfg_data && fields.mfg_data_len > 0) {
        md = fields.mfg_data;
        md_len = fields.mfg_data_len;
    }

    /* Company ID + subtype */
    uint16_t company_id = 0xFFFF;
    uint8_t subtype = 0xFF;

    if (md_len >= 3) {
        company_id = md[0] | (md[1] << 8);
        subtype    = md[2];
    }

    /* Extract name */
    char name[32] = {0};
    if (fields.name && fields.name_len > 0) {
        size_t len = MIN(fields.name_len, sizeof(name) - 1);
        memcpy(name, fields.name, len);
    }

    /* Brand */
    const char *brand = "Unknown";
    if (md_len >= 2) {
        brand = get_brand_from_mfg(md, md_len);
    }

    /* Swift Pair */
    const char *sp_type = NULL;
    if (md_len >= 4) {
        sp_type = get_swift_pair_type(md, md_len);
    }

    /* Apple subtype */
    const char *apple_type = NULL;
    uint8_t apple_subtype = 0xFF;

    if (company_id == 0x004C && md_len >= 3) {
        apple_subtype = md[2];
        apple_type = get_apple_type(md, md_len);
    }

    /* Try to match an existing logical device */
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!devices[i].used) continue;

        bool same_device = false;

        /* 1. Company ID match */
        if (company_id == devices[i].company_id) {

            /* Apple: match subtype */
            if (company_id == 0x004C) {
                if (apple_subtype != 0xFF &&
                    apple_subtype == devices[i].first_subtype) {
                    same_device = true;
                }
            }

            /* Microsoft Swift Pair */
            else if (company_id == 0x0006) {
                same_device = true;
            }

            /* Generic vendor */
            else if (abs(devices[i].last_rssi - rssi) < 15) {
                same_device = true;
            }
        }

        /* 2. Unknown vendor: match name */
        if (!same_device && company_id == 0xFFFF) {
            if (name[0] && devices[i].name[0] &&
                strcmp(name, devices[i].name) == 0) {
                same_device = true;
            }
        }

        /* 3. Fallback: name match */
        if (!same_device && name[0] && devices[i].name[0]) {
            if (strcmp(name, devices[i].name) == 0) {
                same_device = true;
            }
        }

        /* 4. Fallback: adv length + RSSI */
        if (!same_device) {
            if (devices[i].adv_len == event->disc.length_data &&
                abs(devices[i].last_rssi - rssi) < 10) {
                same_device = true;
            }
        }

        /* Update existing device */
        if (same_device) {

            devices[i].addr = *addr;
            devices[i].rssi = rssi;
            devices[i].last_rssi = rssi;
            devices[i].distance = distance;
            devices[i].last_seen = xTaskGetTickCount();

            if (name[0] && devices[i].name[0] == '\0') {
                strncpy(devices[i].name, name, sizeof(devices[i].name)-1);
            }

            strncpy(devices[i].brand, brand, sizeof(devices[i].brand)-1);

            /* DEVICE TYPE CLASSIFICATION */
            if (is_airpods(md, md_len)) {
                strcpy(devices[i].type, "AirPods");
            }
            else if (company_id == 0x004C && apple_subtype == 0x12) {
                strcpy(devices[i].type, "Apple TV");
            }
            else if (company_id == 0x004C) {
                strcpy(devices[i].type, "Apple Device");
            }
            else {
                strcpy(devices[i].type, "Unknown");
            }

            return 0;
        }
    }

    /* NEW DEVICE */
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!devices[i].used) {

            devices[i].used = true;
            devices[i].addr = *addr;
            addr_to_str(&devices[i].addr, devices[i].addr_str, sizeof(devices[i].addr_str));
            devices[i].last_line[0] = '\0'; // force first update
            devices[i].rssi = rssi;
            devices[i].last_rssi = rssi;
            devices[i].distance = distance;
            devices[i].last_seen = xTaskGetTickCount();

            devices[i].company_id = company_id;
            devices[i].first_subtype = subtype;
            devices[i].adv_len = event->disc.length_data;

            strncpy(devices[i].name, name, sizeof(devices[i].name)-1);
            strncpy(devices[i].brand, brand, sizeof(devices[i].brand)-1);

            /* DEVICE TYPE CLASSIFICATION */
            if (is_airpods(md, md_len)) {
                strcpy(devices[i].type, "AirPods");
            }
            else if (company_id == 0x004C && apple_subtype == 0x12) {
                strcpy(devices[i].type, "Apple TV");
            }
            else if (company_id == 0x004C) {
                strcpy(devices[i].type, "Apple Device");
            }
            else {
                strcpy(devices[i].type, "Unknown");
            }

            /* Optional: GATT read */
            if (company_id != 0x004C && company_id != 0x0006) {
                connect_to_device(addr, i);
            }

            break;
        }
    }

    return 0;
}

/* ------------------ BLE Init ------------------ */

static void start_scan(void)
{
    struct ble_gap_disc_params params = {
        .itvl = 0x20,   // 20ms
        .window = 0x20, // 100% duty cycle (idk what that is)
        .filter_policy = BLE_HCI_SCAN_FILT_NO_WL,
        .passive = 0,
        .limited = 0
    };

    ESP_LOGI(TAG, "Starting BLE scan");
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &params, scan_cb, NULL);
}

static void ble_app_on_sync(void)
{
    start_scan();
}

static void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    xTaskCreate(cleanup_task, "cleanup_task", 4096, NULL, 1, NULL);

    nimble_port_freertos_init(host_task);
}
