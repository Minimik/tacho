#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#define BACKGROUND_PATH "/littlefs/tacho.rgb565"
//#define BACKGROUND_PATH "/littlefs/myTacho.rgb565"
#define BACKGROUND_BYTES (240u * 240u * 2u)

#define STORAGE_WIDTH  240
#define STORAGE_HEIGHT 240

esp_err_t storage_init(void);

/*
 * Lädt das komplette Hintergrundbild direkt in den
 * vorhandenen gfx-Framebuffer.
 */
esp_err_t storage_load_background(void);

/*
 * Stellt einen rechteckigen Bereich des Hintergrundbildes
 * direkt im gfx-Framebuffer wieder her.
 *
 * Es wird nur eine kleine Zeile im RAM zwischengespeichert.
 */
esp_err_t storage_restore_rect(
    int x,
    int y,
    int width,
    int height
);