#include "storage.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "gfx.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "storage";


esp_err_t storage_init(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "LittleFS mount failed: %s",
            esp_err_to_name(err)
        );
        return err;
    }

    ESP_LOGI(TAG, "LittleFS mounted");

    return ESP_OK;
}


/*
 * Komplettes Hintergrundbild direkt in den vorhandenen
 * Grafik-Framebuffer laden.
 *
 * Dadurch benötigen wir keinen zweiten 115-kB-Puffer.
 */
esp_err_t storage_load_background(void)
{
    FILE *f = fopen(BACKGROUND_PATH, "rb");

    if (!f) {
        ESP_LOGE(
            TAG,
            "Cannot open background: %s",
            BACKGROUND_PATH
        );

        return ESP_ERR_NOT_FOUND;
    }

    uint16_t *buffer = gfx_buffer();

    size_t n = fread(
        buffer,
        1,
        BACKGROUND_BYTES,
        f
    );

    fclose(f);

    if (n != BACKGROUND_BYTES) {
        ESP_LOGE(
            TAG,
            "Background size error: %u / %u bytes",
            (unsigned)n,
            (unsigned)BACKGROUND_BYTES
        );

        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(
        TAG,
        "Background loaded: %u bytes",
        (unsigned)n
    );

    return ESP_OK;
}


/*
 * Einen rechteckigen Bereich aus dem RAW-RGB565-Bild
 * wieder in den Framebuffer kopieren.
 *
 * Wichtig:
 * Wir lesen NICHT das komplette Bild.
 * Es wird nur eine Zeile im RAM gepuffert.
 *
 * RAM-Verbrauch: maximal 240 * 2 = 480 Bytes.
 */
esp_err_t storage_restore_rect(
    int x,
    int y,
    int width,
    int height
)
{
    if (width <= 0 || height <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Clipping.
     */
    if (x < 0) {
        width += x;
        x = 0;
    }

    if (y < 0) {
        height += y;
        y = 0;
    }

    if (x + width > STORAGE_WIDTH) {
        width = STORAGE_WIDTH - x;
    }

    if (y + height > STORAGE_HEIGHT) {
        height = STORAGE_HEIGHT - y;
    }

    if (width <= 0 || height <= 0) {
        return ESP_OK;
    }

    FILE *f = fopen(BACKGROUND_PATH, "rb");

    if (!f) {
        ESP_LOGE(
            TAG,
            "Cannot open background for restore"
        );

        return ESP_ERR_NOT_FOUND;
    }

    /*
     * Nur eine Zeile puffern.
     */
    uint16_t row[STORAGE_WIDTH];

    uint16_t *framebuffer = gfx_buffer();

    for (int row_y = y; row_y < y + height; row_y++) {

        /*
         * Offset im RAW-RGB565-Bild.
         *
         * Ein Pixel = 2 Bytes.
         */
        long offset =
            ((long)row_y * STORAGE_WIDTH + x)
            * (long)sizeof(uint16_t);

        if (fseek(f, offset, SEEK_SET) != 0) {
            fclose(f);
            return ESP_FAIL;
        }

        size_t bytes =
            (size_t)width * sizeof(uint16_t);

        size_t n = fread(
            row,
            1,
            bytes,
            f
        );

        if (n != bytes) {
            fclose(f);

            ESP_LOGE(
                TAG,
                "Background read error"
            );

            return ESP_ERR_INVALID_SIZE;
        }

        memcpy(
            &framebuffer[row_y * STORAGE_WIDTH + x],
            row,
            bytes
        );
    }

    fclose(f);

    return ESP_OK;
}