#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  240

esp_err_t display_init(void);

esp_lcd_panel_handle_t display_get_panel(void);