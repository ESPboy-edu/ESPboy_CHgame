#pragma once
#include <stdint.h>

// Маппинг кнопок ESPboy (из ESPboyInit.h)
#define A_BUTTON      0x10 // PAD_ACT
#define B_BUTTON      0x20 // PAD_ESC
#define UP_BUTTON     0x02 // PAD_UP
#define DOWN_BUTTON   0x04 // PAD_DOWN
#define LEFT_BUTTON   0x01 // PAD_LEFT
#define RIGHT_BUTTON  0x08 // PAD_RIGHT
#define START_BUTTON  0x80 // PAD_RGT
#define SELECT_BUTTON 0x40 // PAD_LFT

uint8_t chgame_readButtons();

class CHGame {
public:
    void boot();
    void setFrameRate(uint8_t fps);
    bool nextFrame();

    void pollButtons();
    uint8_t buttons() const              { return cur; }
    bool pressed(uint8_t b) const        { return (cur & b) == b; }
    bool anyPressed(uint8_t b) const     { return (cur & b) != 0; }
    bool justPressed(uint8_t b) const    { return (cur & ~prev & b) != 0; }
    bool justReleased(uint8_t b) const   { return (prev & ~cur & b) != 0; }
    uint8_t justPressedMask() const      { return (uint8_t)(cur & ~prev); }
    bool repeat(uint8_t b, uint8_t delay = 18, uint8_t rate = 5) const;
    void clearButtonState()              { prev = cur; }
    bool everyXFrames(uint16_t n) const  { return (frameCount % n) == 0; }

    uint32_t frameCount = 0;
    uint8_t  injected = 0;
    int32_t  lockstep = -1;

private:
    uint8_t  cur = 0, prev = 0;
    uint16_t held[8] = {};
    uint32_t period = 16667, next = 0;
};

extern CHGame CHobj;