// ESPboy_AudioCore.hpp
// Подключается В КОНЦЕ файла Audio.cpp оригинальной игры.

#include <Ticker.h>
#include "ESPboyInit.h"

// Получаем доступ к главному объекту ESPboy, 
// чтобы использовать его встроенный класс управления адресным RGB-светодиодом
extern ESPboyInit myESPboy;

static volatile const Step *fxSteps = nullptr;
static volatile uint8_t fxN = 0, fxI = 0, fxPrio = 0;
static volatile uint16_t fxT = 0;
static Step blipStep;

static bool started = false;
static uint16_t lastHz = 0;
static volatile bool soft;
static uint8_t ledPattern = 0;
static uint16_t ledT = 0;

Ticker audioTicker;

static void myTone(uint16_t hz) {
    if (hz == lastHz) return;
    lastHz = hz;
    if (!hz) noTone(D3); // D3 - пин звука (пьезодинамика) на ESPboy
    else tone(D3, hz);
}

static void audioTick() {
    if (!started) return;
    const Step *s = (const Step *)fxSteps;
    if (!s) { myTone(0); return; }
    
    uint16_t st_hz = pgm_read_byte(&s[fxI].hz);
    uint16_t st_endHz = pgm_read_byte(&s[fxI].endHz);
    uint16_t st_ms = pgm_read_byte(&s[fxI].ms);

    uint16_t hz = (uint16_t)(st_hz * 20);
    uint16_t ms = (uint16_t)(st_ms * 2);
    if (hz && st_endHz) hz = (uint16_t)(hz + ((int32_t)st_endHz * 20 - hz) * fxT / ms);
    
    if (++fxT >= ms) {
        fxT = 0;
        if (++fxI >= fxN) { fxSteps = nullptr; fxPrio = 0; }
    }
    myTone(hz);
}

namespace audio {
bool begin(bool on) {
    pinMode(D3, OUTPUT);
    // pinMode(D4, OUTPUT); <-- УБРАНО. Пин D4 управляет умным RGB-светодиодом через библиотеку, прямой pinMode тут всё ломает.
    
    if (!on) { started = false; lastHz = 1; myTone(0); audioTicker.detach(); return true; }
    if (!started) audioTicker.attach_ms(1, audioTick);
    started = true;
    return true;
}

void setOn(bool on) { begin(on); }

void sfx(Sfx s) {
    if (!started) return;
    const Step* steps = (const Step*)pgm_read_dword(&DEFS[(int)s].steps);
    uint8_t n = pgm_read_byte(&DEFS[(int)s].n);
    uint8_t prio = pgm_read_byte(&DEFS[(int)s].prio);

    if (fxSteps && prio < fxPrio) return;
    
    noInterrupts(); 
    fxSteps = steps; fxN = n; fxI = 0; fxT = 0; fxPrio = prio;
    soft = s >= Sfx::Turn;
    interrupts();   
}

void blip(uint16_t hz, uint16_t ms) {
    if (!started || (fxSteps && fxPrio > 1)) return;
    noInterrupts();
    blipStep.hz = (uint8_t)(hz / 20); blipStep.endHz = 0; blipStep.ms = (uint8_t)(ms / 2);
    fxSteps = &blipStep; fxN = 1; fxI = 0; fxT = 0; fxPrio = 0;
    interrupts();
}

bool playing() { return fxSteps != nullptr; }
void led(Led p) { ledPattern = p; ledT = 0; }

void update() {
    if (!ledPattern) return;
    ledT++;
    bool on = false;
    
    switch (ledPattern) {
        case LED_BLINK:  
            on = ledT < 12; 
            if (ledT > 12) { ledPattern = 0; myESPboy.myLED.setRGB(0,0,0); } 
            break;
        case LED_TRIPLE: 
            on = (ledT % 16) < 8; 
            if (ledT > 48) { ledPattern = 0; myESPboy.myLED.setRGB(0,0,0); } 
            break;
        case LED_PARTY:  
            on = (ledT % 8) < 4; 
            if (ledT > 240) { ledPattern = 0; myESPboy.myLED.setRGB(0,0,0); } 
            break;
    }
    
    if (on) {
        if (ledPattern == LED_PARTY) {
            // "Праздничное" мигание при победе тоже делаем очень тусклым (случайные значения от 0 до 5)
            myESPboy.myLED.setRGB(random(0, 6), random(0, 6), random(0, 6));
        } else {
            // Тусклый зеленый цвет (Red = 0, Green = 5, Blue = 0)
            myESPboy.myLED.setRGB(0, 5, 0); 
        }
    } else {
        myESPboy.myLED.setRGB(0, 0, 0);
    }
}

} // namespace audio