#pragma GCC optimize("Os")
#include <Arduino.h>
#include "CHGame.h"

CHGame arduboy;

void CHGame::boot(ESPboyInit* espboy) {
    espboyRef = espboy;
    cur = prev = espboyRef->getKeys();
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
    cur = (uint8_t)(espboyRef->getKeys() | injected);
    
    static const uint8_t masks[8] = {
        PAD_ACT, PAD_ESC, PAD_UP, PAD_DOWN, 
        PAD_LEFT, PAD_RIGHT, PAD_RGT, PAD_LFT
    };
    
    for (uint8_t i = 0; i < 8; i++) {
        if (cur & masks[i]) { if (held[i] < 0xFFFF) held[i]++; }
        else held[i] = 0;
    }
}

bool CHGame::repeat(uint8_t b, uint8_t delay, uint8_t rate) const {
    static const uint8_t masks[8] = {
        PAD_ACT, PAD_ESC, PAD_UP, PAD_DOWN, 
        PAD_LEFT, PAD_RIGHT, PAD_RGT, PAD_LFT
    };
    for (uint8_t i = 0; i < 8; i++) {
        if (!(b & masks[i])) continue;
        uint16_t h = held[i];
        if (h == 1) return true;
        if (h > delay && ((h - delay) % rate) == 0) return true;
    }
    return false;
}