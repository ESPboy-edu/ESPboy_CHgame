#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

#define GFX_W 128
#define GFX_H 128
#define GFX_FB_STRIDE 64

extern uint8_t* gfx_fb;

// Инициализация спрайта ESPboy в режиме 4 bpp
void gfx_init_espboy(TFT_eSPI* tft);

// Обновление палитры дисплея (принимает массив 16 цветов RGB565)
void gfx_setPalette(const uint16_t* pal16, uint8_t count = 16);

// Отрисовка буфера кадра на экран ESPboy
void gfx_flush_espboy();

// Заглушка ожидания DMA кадра (для синхронного вывода не требуется)
inline void gfx_wait() {}

// Базовые графические примитивы
void gfx_pixel(int x, int y, uint8_t c);
void gfx_hline(int x, int y, int w, uint8_t c);
void gfx_vline(int x, int y, int h, uint8_t c);
void gfx_fillRect(int x, int y, int w, int h, uint8_t c);
void gfx_rect(int x, int y, int w, int h, uint8_t c);
void gfx_line(int x0, int y0, int x1, int y1, uint8_t c);
void gfx_clear(uint8_t c);
void gfx_blit(const uint8_t *spr, int x, int y, uint8_t w, uint8_t h, int8_t trans = -1);

// Вывод стандартного шрифта 5x7 и расчет его ширины
int gfx_text(int x, int y, const char *str, uint8_t c);
int gfx_textWidth(const char *str);