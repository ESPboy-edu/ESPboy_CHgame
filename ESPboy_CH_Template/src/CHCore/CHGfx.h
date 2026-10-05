#pragma once
#include <stdint.h>

#define GFX_W 128
#define GFX_H 128
#define GFX_FB_STRIDE 64
#define GFX_FB_BYTES 8192

extern uint8_t *gfx_fb;

void gfx_init();
void gfx_wait();
void gfx_flushAsync();
uint8_t* gfx_chunkScratch();
void gfx_setPalette(const uint16_t *rgb444, uint8_t n);
uint16_t gfx_paletteOut(uint8_t i);

// Базовые примитивы
void gfx_clear(uint8_t c);
void gfx_pixel(int x, int y, uint8_t c);
void gfx_hline(int x, int y, int w, uint8_t c);
void gfx_vline(int x, int y, int h, uint8_t c);
void gfx_rect(int x, int y, int w, int h, uint8_t c);
void gfx_fillRect(int x, int y, int w, int h, uint8_t c);
void gfx_fillEllipse(int cx, int cy, int rx, int ry, uint8_t c);