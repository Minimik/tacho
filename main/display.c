#include "display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"

#include "esp_log.h"


/* ---------------------------------------------------------
 * GC9A01 Display configuration
 * --------------------------------------------------------- */

#define LCD_HOST       SPI2_HOST

#define PIN_NUM_MOSI   7
#define PIN_NUM_SCLK   6
#define PIN_NUM_CS     10
#define PIN_NUM_DC     2
#define PIN_NUM_RST    3

/*
 * Set to a GPIO number if your display backlight is
 * controlled by the ESP32.
 *
 * Set to -1 if the display module handles the
 * backlight itself.
 */
#define PIN_NUM_BCKL   -1


static const char *TAG = "display";

static esp_lcd_panel_handle_t panel;


/* ---------------------------------------------------------
 * Display initialization
 * --------------------------------------------------------- */

esp_err_t display_init(void)
{
    esp_err_t ret;


    /* -----------------------------------------------------
     * SPI bus
     * ----------------------------------------------------- */

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,

        .quadwp_io_num = -1,
        .quadhd_io_num = -1,

        .max_transfer_sz =
            DISPLAY_WIDTH *
            DISPLAY_HEIGHT *
            sizeof(uint16_t),
    };

    ret = spi_bus_initialize(
        LCD_HOST,
        &buscfg,
        SPI_DMA_CH_AUTO
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "SPI bus initialization failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * LCD SPI IO
     * ----------------------------------------------------- */

    esp_lcd_panel_io_handle_t io = NULL;

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,

        .pclk_hz = 40 * 1000 * 1000,

        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,

        .spi_mode = 0,

        .trans_queue_depth = 4,
    };

    ret = esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST,
        &io_config,
        &io
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "LCD SPI IO initialization failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * GC9A01 panel configuration
     * ----------------------------------------------------- */

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,

        .rgb_ele_order =
            LCD_RGB_ELEMENT_ORDER_RGB,

        .bits_per_pixel = 16,
    };


    /* -----------------------------------------------------
     * Create GC9A01 panel
     * ----------------------------------------------------- */

    ret = esp_lcd_new_panel_gc9a01(
        io,
        &panel_config,
        &panel
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "GC9A01 panel creation failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * Hardware reset
     * ----------------------------------------------------- */

    ret = esp_lcd_panel_reset(panel);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "GC9A01 reset failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * Initialize GC9A01
     * ----------------------------------------------------- */

    ret = esp_lcd_panel_init(panel);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "GC9A01 initialization failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * Orientation
     *
     * Start with this configuration.
     * We can adjust it after the first display test.
     * ----------------------------------------------------- */

    ret = esp_lcd_panel_mirror(
        panel,
        true,
        false
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "GC9A01 mirror configuration failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * Color inversion
     *
     * Some GC9A01 modules need inversion, some don't.
     * Start with true because this matches your previous
     * ST7789 configuration.
     * ----------------------------------------------------- */

    ret = esp_lcd_panel_invert_color(
        panel,
        true
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "GC9A01 color inversion failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * Display ON
     * ----------------------------------------------------- */

    ret = esp_lcd_panel_disp_on_off(
        panel,
        true
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "GC9A01 display ON failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }


    /* -----------------------------------------------------
     * Backlight
     * ----------------------------------------------------- */

#if PIN_NUM_BCKL >= 0

    gpio_config_t bl_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BCKL,
    };

    ret = gpio_config(&bl_config);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Backlight GPIO configuration failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    gpio_set_level(PIN_NUM_BCKL, 1);

#endif


    ESP_LOGI(
        TAG,
        "GC9A01 initialized (%dx%d)",
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT
    );

    return ESP_OK;
}


/* ---------------------------------------------------------
 * Get panel handle
 * --------------------------------------------------------- */

esp_lcd_panel_handle_t display_get_panel(void)
{
    return panel;
}