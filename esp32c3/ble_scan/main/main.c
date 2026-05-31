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
    uint32_t apple_subtypes_seen; 
    bool used;
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

static bool addr_equal(const ble_addr_t *a, const ble_addr_t *b)
{
    return memcmp(a->val, b->val, 6) == 0;
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
    screen_clear();

    printf("ADDR                RSSI   DIST(m)   TYPE           NAME                     BRAND\n");
    printf("-------------------------------------------------------------------------------------------\n");



    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].used) {
            char addr_str[18];
            addr_to_str(&devices[i].addr, addr_str, sizeof(addr_str));

            printf("%-18s  %-5d  %-8.2f  %-13s  %-24s  %s\n",
                addr_str,
                devices[i].rssi,
                devices[i].distance,
                devices[i].type[0] ? devices[i].type : "-",
                devices[i].name[0] ? devices[i].name : "(none)",
                devices[i].brand[0] ? devices[i].brand : "Unknown");

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

static int scan_cb(struct ble_gap_event *event, void *arg)
{
    if (event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    const ble_addr_t *addr = &event->disc.addr;
    int rssi = event->disc.rssi;

    /* Calculate distance */
    float distance = estimate_distance(rssi, -59);  // -59 = default TxPower

    /* Parse advertisement fields */
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    ble_hs_adv_parse_fields(&fields,
                            event->disc.data,
                            event->disc.length_data);

    /* Extract name */
    char name[32] = {0};
    if (fields.name && fields.name_len > 0) {
        size_t len = fields.name_len < sizeof(name) - 1
                     ? fields.name_len
                     : sizeof(name) - 1;
        memcpy(name, fields.name, len);
    }

    /* Extract brand */
    const char *brand = "Unknown";
    if (fields.mfg_data && fields.mfg_data_len >= 2) {
        brand = get_brand_from_mfg(fields.mfg_data, fields.mfg_data_len);
    }

    /* Extract Swift Pair type (if any) */
    const char *sp_type = NULL;
    if (fields.mfg_data && fields.mfg_data_len >= 4) {
        sp_type = get_swift_pair_type(fields.mfg_data, fields.mfg_data_len);
    }

    /* Extract Apple type + subtype (for correlation) */
    const char *apple_type = NULL;
    uint8_t apple_subtype = 0xFF;

    if (fields.mfg_data && fields.mfg_data_len >= 3) {
        uint16_t company_id = fields.mfg_data[0] | (fields.mfg_data[1] << 8);
        if (company_id == 0x004C) {
            apple_subtype = fields.mfg_data[2];
            apple_type = get_apple_type(fields.mfg_data, fields.mfg_data_len);
        }
    }

    /* Check if device already exists */
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].used && addr_equal(&devices[i].addr, addr)) {

            devices[i].rssi = rssi;
            devices[i].distance = distance;
            devices[i].last_seen = xTaskGetTickCount();

            strncpy(devices[i].brand, brand, sizeof(devices[i].brand)-1);
            devices[i].brand[sizeof(devices[i].brand)-1] = '\0';

            /* Multi‑packet correlation happens here */
            decide_device_type(&devices[i], sp_type, apple_type, apple_subtype, name);

            print_table();
            return 0;
        }
    }

    /* New device */
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!devices[i].used) {

            devices[i].used = true;
            devices[i].addr = *addr;
            devices[i].rssi = rssi;
            devices[i].distance = distance;
            devices[i].last_seen = xTaskGetTickCount();
            devices[i].apple_subtypes_seen = 0;   // IMPORTANT

            strncpy(devices[i].name, name, sizeof(devices[i].name)-1);
            devices[i].name[sizeof(devices[i].name)-1] = '\0';

            strncpy(devices[i].brand, brand, sizeof(devices[i].brand)-1);
            devices[i].brand[sizeof(devices[i].brand)-1] = '\0';

            devices[i].type[0] = '\0';

            /* Multi‑packet correlation for new device */
            decide_device_type(&devices[i], sp_type, apple_type, apple_subtype, name);

            print_table();
            break;
        }
    }

    return 0;
}

/* ------------------ BLE Init ------------------ */

static void start_scan(void)
{
    struct ble_gap_disc_params params = {
        .itvl = 0x50,
        .window = 0x30,
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
