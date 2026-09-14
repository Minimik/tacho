#include "gfx.h"
#include <math.h>
#include <string.h>

static uint16_t fb[GFX_WIDTH * GFX_HEIGHT];

void gfx_clear(uint16_t color)
{
    for (int i = 0; i < GFX_WIDTH * GFX_HEIGHT; ++i)
        fb[i] = color;
}

void gfx_pixel(int x, int y, uint16_t color)
{
    if ((unsigned)x < GFX_WIDTH && (unsigned)y < GFX_HEIGHT)
        fb[y * GFX_WIDTH + x] = color;
}

void gfx_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
	
    for (;;) {
        gfx_pixel(x0,y0,color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_circle(int cx, int cy, int r, uint16_t color)
{
    int x = -r, y = 0, err = 2 - 2 * r;
    do {
        gfx_pixel(cx-x, cy+y, color);
        gfx_pixel(cx-y, cy-x, color);
        gfx_pixel(cx+x, cy-y, color);
        gfx_pixel(cx+y, cy+x, color);
        r = err;
        if (r <= y) err += ++y * 2 + 1;
        if (r > x || err > y) err += ++x * 2 + 1;
    } while (x < 0);
}

void gfx_fill_circle(int cx, int cy, int r, uint16_t color)
{
    for (int y = -r; y <= r; ++y) {
        int dx = (int)sqrtf((float)(r*r - y*y));
        for (int x = -dx; x <= dx; ++x)
            gfx_pixel(cx+x, cy+y, color);
    }
}

void gfx_arc(int cx, int cy, int radius, float start_deg, float end_deg,
             uint16_t color, int thickness)
{
    const float step = 0.5f;
    for (float a = start_deg; a <= end_deg; a += step) {
        float rad = a * (float)M_PI / 180.0f;
        float c = cosf(rad), s = sinf(rad);
        for (int t = 0; t < thickness; ++t) {
            int x = (int)lroundf(cx + c * (radius - t));
            int y = (int)lroundf(cy + s * (radius - t));
            gfx_pixel(x, y, color);
        }
    }
}

void gfx_copy_from(const uint16_t *src)
{
    memcpy(fb, src, sizeof(fb));
}

uint16_t *gfx_buffer(void)
{
    return fb;
}

