#pragma GCC optimize("Os")
#include "CHGfx.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ESPboyInit.h"
#include "nbSPI.h"

extern ESPboyInit myESPboy;

// Глобальные указатели на буферы
uint8_t* gfx_fb = nullptr;
static uint8_t* chunk_scratch = nullptr;

static uint16_t palette16[16];
static int clip_x = 0, clip_y = 0, clip_w = GFX_W, clip_h = GFX_H;

#define CHUNK_LINES 8
// Указатели на двойной буфер DMA вместо статического массива
static uint16_t* dma_buf[2] = {nullptr, nullptr};

void gfx_setClip(int x, int y, int w, int h) { clip_x = x; clip_y = y; clip_w = w; clip_h = h; }
void gfx_resetClip() { clip_x = 0; clip_y = 0; clip_w = GFX_W; clip_h = GFX_H; }
void gfx_getClip(int *x, int *y, int *w, int *h) { *x = clip_x; *y = clip_y; *w = clip_w; *h = clip_h; }

void gfx_init() {
    // Выделяем память под все буферы строго в момент инициализации (~13 КБ в сумме)
    if (!gfx_fb) gfx_fb = (uint8_t*)malloc(GFX_FB_BYTES);
    if (!chunk_scratch) chunk_scratch = (uint8_t*)malloc(1024);
    if (!dma_buf[0]) dma_buf[0] = (uint16_t*)malloc(GFX_W * CHUNK_LINES * 2);
    if (!dma_buf[1]) dma_buf[1] = (uint16_t*)malloc(GFX_W * CHUNK_LINES * 2);
}

void gfx_wait() {
    while(nbSPI_isBusy()) { yield(); }
}

uint8_t* gfx_chunkScratch() {
    if (!chunk_scratch) chunk_scratch = (uint8_t*)malloc(1024);
    return chunk_scratch;
}

void gfx_setPalette(const uint16_t *colors, uint8_t n) {
    for(uint8_t i = 0; i < n && i < 16; i++) {
        uint16_t color565 = colors[i];
        palette16[i] = (color565 >> 8) | (color565 << 8); 
    }
}

uint16_t gfx_paletteOut(uint8_t i) { return palette16[i & 15]; }

void gfx_flushAsync() {
    // Если память не выделилась (Out of Memory), прерываем отрисовку
    if (!gfx_fb || !dma_buf[0] || !dma_buf[1]) return;
    
    while(nbSPI_isBusy()) { yield(); } 
    
    myESPboy.tft.setWindow(0, 0, GFX_W - 1, GFX_H - 1);
    
    int fb_idx = 0;
    int buf_idx = 0;
    
    for (int y = 0; y < GFX_H; y += CHUNK_LINES) {
        uint16_t* ptr = dma_buf[buf_idx];
        
        for (int i = 0; i < (GFX_W * CHUNK_LINES) / 2; i++) {
            uint8_t pair = gfx_fb[fb_idx++];
            *ptr++ = palette16[pair & 0x0F];
            *ptr++ = palette16[pair >> 4];
        }
        
        while(nbSPI_isBusy()) { yield(); } 
        
        nbSPI_writeBytes((uint8_t*)dma_buf[buf_idx], GFX_W * CHUNK_LINES * 2);
        buf_idx ^= 1; 
    }
}

void gfx_clear(uint8_t c) { if (gfx_fb) memset(gfx_fb, (c << 4) | (c & 0x0F), GFX_FB_BYTES); }

void gfx_pixel(int x, int y, uint8_t c) {
    if (x < clip_x || x >= clip_x + clip_w || y < clip_y || y >= clip_y + clip_h || !gfx_fb) return;  
    int idx = y * GFX_FB_STRIDE + (x >> 1);
    if (x & 1) gfx_fb[idx] = (gfx_fb[idx] & 0x0F) | (c << 4); 
    else       gfx_fb[idx] = (gfx_fb[idx] & 0xF0) | (c & 0x0F);
}


void gfx_hline(int x, int y, int w, uint8_t c) { for (int i = 0; i < w; i++) gfx_pixel(x + i, y, c); }
void gfx_vline(int x, int y, int h, uint8_t c) { for (int i = 0; i < h; i++) gfx_pixel(x, y + i, c); }
void gfx_rect(int x, int y, int w, int h, uint8_t c) {
    gfx_hline(x, y, w, c); gfx_hline(x, y + h - 1, w, c);
    gfx_vline(x, y, h, c); gfx_vline(x + w - 1, y, h, c);
}
void gfx_fillRect(int x, int y, int w, int h, uint8_t c) { for (int i = 0; i < h; i++) gfx_hline(x, y + i, w, c); }
void gfx_fillEllipse(int cx, int cy, int rx, int ry, uint8_t c) { gfx_fillRect(cx - rx, cy - ry, rx * 2, ry * 2, c); }