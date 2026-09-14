#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_random.h"

#include "display.h"
#include "gfx.h"
#include "gauge.h"
#include "storage.h"


static const char *TAG = "tacho";


void app_main(void)
{
    ESP_LOGI(TAG, "Starting tachometer");

    /*
     * Display initialisieren.
     */
    ESP_ERROR_CHECK(display_init());

    /*
     * LittleFS initialisieren.
     */
    ESP_ERROR_CHECK(storage_init());

    /*
     * Hintergrund direkt in den vorhandenen
     * gfx-Framebuffer laden.
     *
     * KEIN zweiter 115-kB-Puffer mehr.
     */
    ESP_ERROR_CHECK(storage_load_background());

    esp_lcd_panel_handle_t panel = display_get_panel();

    /*
     * Demo-Signal.
     * Später durch echte Geschwindigkeit ersetzen.
     */
    float speed = 0.0f;
    float target = 0.0f;

    int direction = 1;

    while (1) {

        /*
         * Demo-Signal.
         */
        target = ( esp_random() % 2001 + 3000 ); // direction * 1.0f;

        if (target >= 5000.0f) {
            target = 5000.0f;
            direction = -1;
        }

        if (target <= 0.0f) {
            target = 0.0f;
            direction = 1;
        }


        /*
         * Mechanische, weiche Nadelbewegung.
         */
        speed = (target); //- speed) * 0.08f;


        /*
         * Alten dynamischen Bereich restaurieren
         * und anschließend die aktuelle Anzeige zeichnen.
         */
        gauge_render(speed);


        /*
         * Kompletten Frame zum GC9A01 übertragen.
         */
        ESP_ERROR_CHECK(
            esp_lcd_panel_draw_bitmap(
                panel,
                0,
                0,
                DISPLAY_WIDTH,
                DISPLAY_HEIGHT,
                gfx_buffer()
            )
        );


        /*
         * Bei 27 MHz SPI ist ein echter 60-Hz-Vollbild-
         * Transfer ohnehin nicht möglich.
         *
         * 40 ms sind zunächst ein guter stabiler Wert.
         */
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}