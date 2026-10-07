#pragma GCC optimize("Os", "no-ipa-sra")
#include <Arduino.h>
#include "Audio.h"

struct Step { uint8_t hz, endHz, ms; };

#define S(hz, end, ms) { (uint8_t)(((hz) + 10) / 20), (uint8_t)(((end) + 10) / 20), (uint8_t)(((ms) + 1) / 2) }
#define REST(ms)       { 0, 0, (uint8_t)(((ms) + 1) / 2) }

static const Step CURSOR[] PROGMEM  = { S(2100, 0, 10) };
static const Step SELECT[] PROGMEM  = { S(1700, 0, 18), S(2600, 0, 30) };
static const Step DENY[] PROGMEM    = { S(900, 650, 70) };
static const Step LAND[] PROGMEM    = { S(2400, 1100, 14), REST(8), S(3300, 0, 16) };
static const Step LIFT[] PROGMEM    = { S(1500, 2600, 40) };
static const Step COIN[] PROGMEM    = { S(2800, 0, 10), S(3700, 0, 28) };
static const Step SCORE[] PROGMEM   = {
    S(1568, 0, 45), S(2093, 0, 45), S(2637, 0, 45), S(3136, 0, 45), S(4186, 0, 60),
    S(3136, 0, 30), S(4186, 0, 30), S(3136, 0, 30), S(4186, 0, 120) };
static const Step WHOOSH[] PROGMEM  = { S(1200, 3800, 90) };
static const Step KNOCK[] PROGMEM   = { S(900, 500, 30), REST(90), S(900, 500, 30) };
static const Step BOOM[] PROGMEM    = {
    S(3800, 0, 6), S(700, 0, 8), S(3200, 0, 6), S(600, 0, 8), S(2800, 0, 6), S(520, 0, 10),
    S(2400, 0, 6), S(480, 0, 12), S(1400, 400, 200) };
static const Step MATCH[] PROGMEM   = {
    S(1568, 0, 50), S(2093, 0, 50), S(2637, 0, 50), S(3136, 0, 90),
    S(2093, 0, 40), S(2637, 0, 40), S(2093, 0, 40), S(2637, 0, 40),
    S(3136, 0, 40), S(4186, 0, 40), S(3136, 0, 40), S(4186, 0, 40),
    S(2000, 4200, 220) };
static const Step WIN[] PROGMEM     = {
    S(2093, 0, 110), S(2637, 0, 110), S(3136, 0, 110), S(4186, 0, 220), REST(60),
    S(3520, 0, 110), S(4186, 0, 330) };
static const Step LOSE[] PROGMEM    = { S(1568, 1480, 260), S(1480, 1397, 260), S(1397, 1319, 260), S(1319, 1210, 350),
                                S(1210, 1100, 350) };
static const Step TURN[] PROGMEM    = { S(2637, 0, 40), S(3520, 0, 90) };
static const Step TITLE[] PROGMEM   = {
    S(1568, 0, 90), S(2093, 0, 90), S(2637, 0, 90), S(3136, 0, 180), REST(40),
    S(2637, 0, 90), S(3136, 0, 360) };

struct SfxDef { const Step *steps; uint8_t n, prio; };
#define DEF(a, p) { a, (uint8_t)(sizeof(a) / sizeof(a[0])), p }

static const SfxDef DEFS[(int)Sfx::COUNT] PROGMEM = {
    DEF(CURSOR, 0), DEF(SELECT, 1), DEF(DENY, 1), DEF(LAND, 1), DEF(LIFT, 1), DEF(COIN, 1),
    DEF(SCORE, 3), DEF(WHOOSH, 1), DEF(KNOCK, 2), DEF(BOOM, 3),
    DEF(MATCH, 4), DEF(WIN, 4), DEF(LOSE, 4), DEF(TURN, 1), DEF(TITLE, 2),
};

#include "../CHCore/ESPboy_AudioCore.hpp"