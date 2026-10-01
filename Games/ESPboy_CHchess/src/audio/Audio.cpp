// A small sound sequencer for the CHGame piezo (from CHBlackjack).
//
// Effects play on TIM1 channel 2 (PB10), stepped by the core's 1 kHz
// SysTick hook (osSystickHandler), so no other timer is used. One pin plays
// one note at a time; a higher-priority effect is never cut off by a lower
// one. tools/audio/preview.py renders every effect to WAV from this file.
#pragma GCC optimize("Os")
#include <Arduino.h>
#include <Ticker.h>
#include "Audio.h"

struct Step { uint16_t hz, endHz, ms; };        

#define S(hz, end, ms) { (uint16_t)(hz), (uint16_t)(end), (uint16_t)(ms) }
#define REST(ms)       { 0, 0, (uint16_t)(ms) }

// --- Здесь остаются ваши PROGMEM массивы шагов (CURSOR, SELECT и т.д.) ---
// Оставляем те же массивы, что были в предыдущем шаге
static const PROGMEM Step CURSOR[]  = { S(2100, 0, 10) };
static const PROGMEM Step SELECT[]  = { S(1700, 0, 18), S(2600, 0, 30) };
static const PROGMEM Step DENY[]    = { S(900, 650, 70) };
static const PROGMEM Step LAND[]    = { S(2400, 1100, 14), REST(8), S(3300, 0, 16) };
static const PROGMEM Step HOP[]     = { S(1500, 3300, 80) };
static const PROGMEM Step CAPTURE[] = {
    S(3800, 0, 6), S(700, 0, 8), S(3200, 0, 6), S(600, 0, 8), S(2800, 0, 6), S(520, 0, 10),
    S(2400, 0, 6), S(480, 0, 12),
    S(2600, 2200, 70), S(2400, 2000, 70), S(2200, 1800, 70), S(2000, 1600, 70), S(1800, 1400, 70),
    S(1600, 1200, 80), S(1400, 900, 110) };
static const PROGMEM Step COIN[]    = { S(2800, 0, 10), S(3700, 0, 28) };
static const PROGMEM Step CHECK[]   = { S(2637, 0, 70), S(1976, 0, 70), S(2637, 0, 70), S(1976, 0, 120) };
static const PROGMEM Step CASTLE[]  = { S(2400, 1100, 14), REST(40), S(1400, 3200, 70), REST(30), S(2400, 1100, 14), REST(8), S(3300, 0, 16) };
static const PROGMEM Step PROMOTE[] = {
    S(1568, 0, 45), S(2093, 0, 45), S(2637, 0, 45), S(3136, 0, 45), S(4186, 0, 60),
    S(3136, 0, 30), S(4186, 0, 30), S(3136, 0, 30), S(4186, 0, 120) };
static const PROGMEM Step WHOOSH[]  = { S(1200, 3800, 90) };
static const PROGMEM Step FLIP[]    = { S(2300, 0, 8), REST(5), S(3300, 0, 12) };
static const PROGMEM Step MATE[]    = {
    S(1568, 0, 50), S(2093, 0, 50), S(2637, 0, 50), S(3136, 0, 90),
    S(2093, 0, 40), S(2637, 0, 40), S(2093, 0, 40), S(2637, 0, 40),
    S(3136, 0, 40), S(4186, 0, 40), S(3136, 0, 40), S(4186, 0, 40),
    S(2000, 4200, 220) };
static const PROGMEM Step WIN[]     = {
    S(2093, 0, 110), S(2637, 0, 110), S(3136, 0, 110), S(4186, 0, 220), REST(60),
    S(3520, 0, 110), S(4186, 0, 330) };
static const PROGMEM Step LOSE[]    = { S(1568, 1480, 260), S(1480, 1397, 260), S(1397, 1319, 260), S(1319, 1100, 700) };
static const PROGMEM Step DRAW[]    = { S(1760, 0, 70), REST(40), S(1760, 0, 70), REST(40), S(1319, 0, 160) };
static const PROGMEM Step TURN[]    = { S(2637, 0, 40), S(3520, 0, 90) };
static const PROGMEM Step TITLE[]   = {
    S(1568, 0, 90), S(2093, 0, 90), S(2637, 0, 90), S(3136, 0, 180), REST(40),
    S(2637, 0, 90), S(3136, 0, 360) };
static const PROGMEM Step TICK[]    = { S(1100, 0, 3) };
static const PROGMEM Step TOCK[]    = { S(850, 0, 3) };

struct SfxDef { const Step *steps; uint8_t n, prio; };
#define DEF(a, p) { a, (uint8_t)(sizeof(a) / sizeof(a[0])), p }
static const PROGMEM SfxDef DEFS[(int)Sfx::COUNT] = {
    DEF(CURSOR, 0), DEF(SELECT, 1), DEF(DENY, 1), DEF(LAND, 1), DEF(HOP, 1), DEF(CAPTURE, 2),
    DEF(COIN, 1), DEF(CHECK, 3), DEF(CASTLE, 2), DEF(PROMOTE, 3), DEF(WHOOSH, 1), DEF(FLIP, 1),
    DEF(MATE, 4), DEF(WIN, 4), DEF(LOSE, 4), DEF(DRAW, 4), DEF(TURN, 1), DEF(TITLE, 2),
    DEF(TICK, 0), DEF(TOCK, 0),
};

// --- Sequencer state ---
static volatile const Step *fxSteps = nullptr;
static volatile uint8_t fxN = 0, fxI = 0, fxPrio = 0;
static volatile uint16_t fxT = 0;

static bool started = false;
static uint16_t lastHz = 0;
static volatile bool soft;
static uint8_t ledPattern = 0;
static uint16_t ledT = 0;

Ticker audioTicker;

static void myTone(uint16_t hz) {
    if (hz == lastHz) return;
    lastHz = hz;
    if (!hz) {
        noTone(D3); // D3 - это SOUNDPIN на ESPboy
    } else {
        tone(D3, hz);
    }
}

static void audioTick() {
    if (!started) return;
    const Step *s = (const Step *)fxSteps;
    if (!s) { myTone(0); return; }
    
    uint16_t st_hz = pgm_read_word(&s[fxI].hz);
    uint16_t st_endHz = pgm_read_word(&s[fxI].endHz);
    uint16_t st_ms = pgm_read_word(&s[fxI].ms);

    uint16_t hz = st_hz;
    if (hz && st_endHz) hz = (uint16_t)(st_hz + ((int32_t)st_endHz - st_hz) * fxT / st_ms);
    
    if (++fxT >= st_ms) {
        fxT = 0;
        if (++fxI >= fxN) { fxSteps = nullptr; fxPrio = 0; }
    }
    myTone(hz);
}

namespace audio {
bool begin(bool on) {
    pinMode(D3, OUTPUT);
    pinMode(D4, OUTPUT); // D4 - системный LED ESPboy
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
    
    noInterrupts(); // Аналог __disable_irq() для ESP8266
    fxSteps = steps; fxN = n; fxI = 0; fxT = 0; fxPrio = prio;
    soft = s >= Sfx::Tick;
    interrupts();   // Аналог __enable_irq()
}

bool playing() { return fxSteps != nullptr; }
void led(Led p) { ledPattern = p; ledT = 0; }

void update() {
    if (!ledPattern) return;
    ledT++;
    bool on = false;
    switch (ledPattern) {
        case LED_BLINK:  on = ledT < 12; if (ledT > 12) ledPattern = 0; break;
        case LED_TRIPLE: on = (ledT % 16) < 8; if (ledT > 48) ledPattern = 0; break;
        case LED_PARTY:  on = (ledT % 8) < 4; if (ledT > 240) ledPattern = 0; break;
    }
    digitalWrite(D4, on ? LOW : HIGH);
}
} // namespace audio