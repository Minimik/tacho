idf_component_register(
    SRCS "main.c" "display.c" "gfx.c" "gauge.c" "storage.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_lcd_gc9a01 driver littlefs
)

# Create the LittleFS image from the project assets/ directory.
littlefs_create_partition_image(storage ../assets FLASH_IN_PROJECT)
