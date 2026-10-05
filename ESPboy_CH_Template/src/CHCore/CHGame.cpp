#pragma GCC optimize("Os")
#include <Arduino.h>
#include "CHGame.h"
#include "../../lib/ESPboyInit.h" // Путь к ядру ESPboy

extern ESPboyInit myESPboy;
CHGame arduboy;

// Универсальный опросник кнопок
uint8_t chgame_readButtons() {
    return myESPboy.getKeys();
}

void CHGame::boot() {
    cur = prev = chgame_readButtons();
}

void CHGame::setFrameRate(uint8_t fps) {
    period = 1000000u / fps;
    next = micros() + period;
}

bool CHGame::nextFrame() {
    if (lockstep >= 0) {
        if (lockstep == 0) return false;
        lockstep--;
        frameCount++;
        return true;
    }
    uint32_t now = micros();
    if ((int32_t)(now - next) < 0) return false;
    next += period;
    if ((int32_t)(now - next) > (int32_t)(3 * period)) next = now + period;
    frameCount++;
    return true;
}

void CHGame::pollButtons() {
    prev = cur;
    cur = (uint8_t)(chgame_readButtons() | injected);
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t mask = (1u << i);
        // Маппинг битов
        if (i==0) mask = A_BUTTON; else if (i==1) mask = B_BUTTON;
        else if (i==2) mask = UP_BUTTON; else if (i==3) mask = DOWN_BUTTON;
        else if (i==4) mask = LEFT_BUTTON; else if (i==5) mask = RIGHT_BUTTON;
        else if (i==6) mask = START_BUTTON; else if (i==7) mask = SELECT_BUTTON;
        
        if (cur & mask) { if (held[i] < 0xFFFF) held[i]++; }
        else held[i] = 0;
    }
}

bool CHGame::repeat(uint8_t b, uint8_t delay, uint8_t rate) const {
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t mask = (1u << i);
        if (i==0) mask = A_BUTTON; else if (i==1) mask = B_BUTTON;
        else if (i==2) mask = UP_BUTTON; else if (i==3) mask = DOWN_BUTTON;
        else if (i==4) mask = LEFT_BUTTON; else if (i==5) mask = RIGHT_BUTTON;
        else if (i==6) mask = START_BUTTON; else if (i==7) mask = SELECT_BUTTON;
        
        if (!(b & mask)) continue;
        uint16_t h = held[i];
        if (h == 1) return true;
        if (h > delay && ((h - delay) % rate) == 0) return true;
    }
    return false;
}