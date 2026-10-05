#pragma GCC optimize("Os")
#include "CHGfx.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../../lib/ESPboyInit.h"
#include "../../lib/nbSPI.h" // Ваше ускорение!

extern ESPboyInit myESPboy;
TFT_eSprite* spr = nullptr;

uint8_t* gfx_fb = nullptr;
static uint8_t* chunk_scratch = nullptr;
static uint16_t palette16[16];

void gfx_init() {
    if (gfx_fb == nullptr) gfx_fb = new uint8_t[GFX_FB_BYTES];
    if (chunk_scratch == nullptr) chunk_scratch = new uint8_t[1024];
    
    if (spr == nullptr) {
        spr = new TFT_eSprite(&myESPboy.tft);
        spr->setColorDepth(16);
        spr->createSprite(GFX_W, GFX_H);
    }
}

void gfx_wait() {
    // В ESP8266 с nbSPI здесь мы ждем окончания фоновой отправки
    while(nbSPI_isBusy()) { yield(); }
}

uint8_t* gfx_chunkScratch() {
    if (chunk_scratch == nullptr) chunk_scratch = new uint8_t[1024];
    return chunk_scratch;
}

void gfx_setPalette(const uint16_t *rgb444, uint8_t n) {
    for(uint8_t i = 0; i < n && i < 16; i++) {
        uint16_t c = rgb444[i];
        uint16_t r = (c >> 8) & 0xF;
        uint16_t g = (c >> 4) & 0xF;
        uint16_t b = c & 0xF;
        palette16[i] = myESPboy.tft.color565(r * 17, g * 17, b * 17);
    }
}

uint16_t gfx_paletteOut(uint8_t i) { return palette16[i & 15]; }

void gfx_flushAsync() {
    if (gfx_fb == nullptr || spr == nullptr) return;
    uint16_t* ptr = (uint16_t*)spr->frameBuffer();
    
    // Конвертация 4bpp -> 16bpp прямо в буфер спрайта
    for (int i = 0; i < 8192; i++) {
        uint8_t pair = gfx_fb[i];
        *ptr++ = palette16[pair >> 4];   // Левый пиксель
        *ptr++ = palette16[pair & 0x0F]; // Правый пиксель
    }
    
    // Асинхронный быстрый вывод через nbSPI (эмуляция DMA)
    if (!nbSPI_isBusy()) {
        myESPboy.tft.setWindow(0, 0, 127, 127);
        nbSPI_writeBytes((uint8_t*)spr->frameBuffer(), 128 * 128 * 2);
    }
}

// === Реализация базовых примитивов ===
void gfx_clear(uint8_t c) {
    if (gfx_fb) memset(gfx_fb, (c << 4) | (c & 0x0F), GFX_FB_BYTES);
}

void gfx_pixel(int x, int y, uint8_t c) {
    if (x < 0 || x >= GFX_W || y < 0 || y >= GFX_H || !gfx_fb) return;
    int idx = y * GFX_FB_STRIDE + (x >> 1);
    if (x & 1) gfx_fb[idx] = (gfx_fb[idx] & 0xF0) | (c & 0x0F);
    else       gfx_fb[idx] = (gfx_fb[idx] & 0x0F) | (c << 4);
}

void gfx_hline(int x, int y, int w, uint8_t c) {
    for (int i = 0; i < w; i++) gfx_pixel(x + i, y, c);
}

void gfx_vline(int x, int y, int h, uint8_t c) {
    for (int i = 0; i < h; i++) gfx_pixel(x, y + i, c);
}

void gfx_rect(int x, int y, int w, int h, uint8_t c) {
    gfx_hline(x, y, w, c); gfx_hline(x, y + h - 1, w, c);
    gfx_vline(x, y, h, c); gfx_vline(x + w - 1, y, h, c);
}

void gfx_fillRect(int x, int y, int w, int h, uint8_t c) {
    for (int i = 0; i < h; i++) gfx_hline(x, y + i, w, c);
}

// Заглушка: на ESP8266 мы рисуем скругленный квадрат вместо сложного эллипса для экономии ресурсов
void gfx_fillEllipse(int cx, int cy, int rx, int ry, uint8_t c) {
    gfx_fillRect(cx - rx, cy - ry, rx * 2, ry * 2, c);
}