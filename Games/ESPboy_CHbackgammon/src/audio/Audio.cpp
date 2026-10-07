// A small sound sequencer for the CHGame piezo (from CHBlackjack).
//
// Effects play on TIM1 channel 2 (PB10), stepped by the core's 1 kHz
// SysTick hook (osSystickHandler), so no other timer is used. One pin plays
// one note at a time; a higher-priority effect is never cut off by a lower
// one. tools/audio/preview.py renders every effect to WAV from this file.
#pragma GCC optimize("Os", "no-ipa-sra")
#include <Arduino.h>
#include "Audio.h"

#ifdef CHSIM
// The simulator is silent: same interface, no hardware. It remembers the
// last effect so scripts and tests can check what would have sounded.
namespace audio {
static bool simOn = true;
uint8_t simLast = 0xFF;
bool begin(bool on) { simOn = on; return true; }
void setOn(bool on) { simOn = on; }
void sfx(Sfx s) { if (simOn) simLast = (uint8_t)s; }
bool playing() { return false; }
void update() {}
void led(Led) {}
}
#else

// An effect step in three bytes: the pitch and the pitch it sweeps to (0:
// none) in 20 Hz units, and its length in 2 ms units; pitch 0 = a rest.
// (20 Hz is under half a percent of these pitches: no ear hears it on a
// piezo, and the tables are half the size.)
struct Step { uint8_t hz, endHz, ms; };

#define S(hz, end, ms) { (uint8_t)(((hz) + 10) / 20), (uint8_t)(((end) + 10) / 20), (uint8_t)(((ms) + 1) / 2) }
#define REST(ms)       { 0, 0, (uint8_t)(((ms) + 1) / 2) }

static const Step CURSOR[]  = { S(2100, 0, 10) };
static const Step SELECT[]  = { S(1700, 0, 18), S(2600, 0, 30) };
static const Step DENY[]    = { S(900, 650, 70) };
// A checker set down: a knock and a higher tick.
static const Step LAND[]    = { S(2400, 1100, 14), REST(8), S(3300, 0, 16) };
static const Step LIFT[]    = { S(1500, 2600, 40) };
// A blot hit - tones alternating high and low read as noise on a piezo, the
// lows lengthening as it lands - then the checker sent flying: falling
// swoops, about as long as its trip to the bar.
static const Step HIT[]     = {
    S(3800, 0, 6), S(700, 0, 8), S(3200, 0, 6), S(600, 0, 8), S(2800, 0, 6), S(520, 0, 10),
    S(2400, 0, 6), S(480, 0, 12),
    S(2600, 2200, 70), S(2400, 2000, 70), S(2200, 1800, 70), S(2000, 1600, 70), S(1800, 1400, 80),
    S(1600, 1100, 110) };
// A checker borne off: a chip dropped in the tray.
static const Step COIN[]    = { S(2800, 0, 10), S(3700, 0, 28) };
// Dice across the board: clicks at uneven pitches, slowing, and two knocks
// as they come to rest.
static const Step RATTLE[]  = {
    S(2900, 0, 5), REST(22), S(2300, 0, 5), REST(26), S(3300, 0, 5), REST(30), S(2500, 0, 5), REST(38),
    S(3000, 0, 5), REST(48), S(2200, 0, 6), REST(60), S(2600, 1300, 14), REST(60), S(2200, 1100, 16) };
static const Step DOUBLES[] = {
    S(1568, 0, 45), S(2093, 0, 45), S(2637, 0, 45), S(3136, 0, 45), S(4186, 0, 60),
    S(3136, 0, 30), S(4186, 0, 30), S(3136, 0, 30), S(4186, 0, 120) };
static const Step PICKUP[]  = { S(2300, 0, 8), REST(5), S(3300, 0, 12), REST(20), S(1400, 3200, 60) };
static const Step WHOOSH[]  = { S(1200, 3800, 90) };
static const Step NOMOVE[]  = { S(2637, 0, 70), S(1976, 0, 70), S(1568, 0, 70), S(1175, 0, 160) };
// CHBlackjack's BLACKJACK fanfare: the signature win, for a gammon.
static const Step GAMMON[]  = {
    S(1568, 0, 50), S(2093, 0, 50), S(2637, 0, 50), S(3136, 0, 90),
    S(2093, 0, 40), S(2637, 0, 40), S(2093, 0, 40), S(2637, 0, 40),
    S(3136, 0, 40), S(4186, 0, 40), S(3136, 0, 40), S(4186, 0, 40),
    S(2000, 4200, 220) };
// Victory: a short tune (was a Playtune score in CHBlackjack).
static const Step WIN[]     = {
    S(2093, 0, 110), S(2637, 0, 110), S(3136, 0, 110), S(4186, 0, 220), REST(60),
    S(3520, 0, 110), S(4186, 0, 330) };
static const Step LOSE[]    = { S(1568, 1480, 260), S(1480, 1397, 260), S(1397, 1319, 260), S(1319, 1210, 350),
                                S(1210, 1100, 350) };
static const Step TURN[]    = { S(2637, 0, 40), S(3520, 0, 90) };
static const Step TITLE[]   = {
    S(1568, 0, 90), S(2093, 0, 90), S(2637, 0, 90), S(3136, 0, 180), REST(40),
    S(2637, 0, 90), S(3136, 0, 360) };
// The CPU's clock while it thinks: the faintest clicks (played soft).
static const Step TICK[]    = { S(1100, 0, 3) };
static const Step TOCK[]    = { S(850, 0, 3) };

struct SfxDef { const Step *steps; uint8_t n, prio; };
#define DEF(a, p) { a, (uint8_t)(sizeof(a) / sizeof(a[0])), p }
static const SfxDef DEFS[(int)Sfx::COUNT] = {
    DEF(CURSOR, 0), DEF(SELECT, 1), DEF(DENY, 1), DEF(LAND, 1), DEF(LIFT, 1), DEF(HIT, 2),
    DEF(COIN, 1), DEF(RATTLE, 2), DEF(DOUBLES, 3), DEF(PICKUP, 1), DEF(WHOOSH, 1), DEF(NOMOVE, 3),
    DEF(GAMMON, 4), DEF(WIN, 4), DEF(LOSE, 4), DEF(TURN, 1), DEF(TITLE, 2),
    DEF(TICK, 0), DEF(TOCK, 0),
};


#include "../CHCore/ESPboy_AudioCore.hpp"

#endif  // CHSIM
