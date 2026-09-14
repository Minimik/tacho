#pragma once
#include <stdint.h>
#include <stdbool.h>

#define GFX_WIDTH  240
#define GFX_HEIGHT 240

#define RGB565(r,g,b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

void gfx_clear(uint16_t color);
void gfx_pixel(int x, int y, uint16_t color);
void gfx_line(int x0, int y0, int x1, int y1, uint16_t color);
void gfx_circle(int cx, int cy, int radius, uint16_t color);
void gfx_fill_circle(int cx, int cy, int radius, uint16_t color);
void gfx_arc(int cx, int cy, int radius, float start_deg, float end_deg,
             uint16_t color, int thickness);
void gfx_copy_from(const uint16_t *src);
uint16_t *gfx_buffer(void);
