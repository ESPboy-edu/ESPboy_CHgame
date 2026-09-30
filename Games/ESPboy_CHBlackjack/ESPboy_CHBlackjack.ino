#include "config.h"
#include <EEPROM.h>
#include "src/gfx/ESPboyGfx.h"
#include "src/CHGame.h"
#include "src/gfx/Palette.h"
#include "src/states/Screens.h"

#include "ESPboyInit.h"

ESPboyInit myESPboy;

void setup() {
    Serial.begin(115200);
    myESPboy.begin("CHblackjack");

    Serial.println();
    Serial.println(ESP.getFreeHeap()); 
    
    EEPROM.begin(512);

    // Инициализация графического движка ESPboy (4 bpp)
    gfx_init_espboy(&myESPboy.tft);

    arduboy.boot(&myESPboy);
    pal::init();
    screens::begin();
    arduboy.setFrameRate(CHBJ_FPS);
}

void loop() {
    if (!arduboy.nextFrame()) return;

    uint8_t ticks = 0;
    do {
        arduboy.pollButtons();
        pal::tick();
        screens::update();
    } while (++ticks < 3 && arduboy.nextFrame());

    pal::commit();
    screens::render(arduboy.frameCount);
    
    // Прямой вывод 4bpp кадрового буфера с установленной палитрой
    gfx_flush_espboy();
}
