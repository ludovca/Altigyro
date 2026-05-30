#include <stdio.h>
#include <string.h>

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
    char name[32];
    char brand[32];
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

    printf("ADDR                RSSI   NAME                     BRAND\n");
    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].used) {
            char addr_str[18];
            addr_to_str(&devices[i].addr, addr_str, sizeof(addr_str));

            printf("%-18s  %-5d  %-24s  %s\n",
                   addr_str,
                   devices[i].rssi,
                   devices[i].name[0] ? devices[i].name : "(none)",
                   devices[i].brand[0] ? devices[i].brand : "Unknown");
        }
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

    /* Check if device already exists */
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].used && addr_equal(&devices[i].addr, addr)) {

            devices[i].rssi = rssi;
            strncpy(devices[i].brand, brand, sizeof(devices[i].brand)-1);

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
            strncpy(devices[i].name, name, sizeof(devices[i].name)-1);
            strncpy(devices[i].brand, brand, sizeof(devices[i].brand)-1);

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

    nimble_port_freertos_init(host_task);
}
