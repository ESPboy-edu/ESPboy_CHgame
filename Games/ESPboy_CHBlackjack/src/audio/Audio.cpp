#include <Arduino.h>
#include "Audio.h"
#include "Music.h"
#include "../../ESPboyInit.h"

extern ESPboyInit myESPboy;

struct Step { uint16_t hz, endHz, ms; };

static volatile const Step *fxSteps = nullptr;
static volatile uint8_t fxN = 0, fxI = 0, fxPrio = 0;
static volatile uint16_t fxT = 0;
static Step blipStep;

static volatile const uint8_t *score = nullptr, *scorePos = nullptr;
static volatile uint16_t scoreWait = 0;
static volatile bool scoreLoops = true;
static volatile uint8_t notes[4];

static uint8_t arpT = 0, arpCh = 0, leadHold = 0;
static const uint8_t ARP_MS = 6;
static const uint8_t LEAD_HOLD_MS = 20;

static bool started = false, isMuted = false;
static uint8_t mode = 1;
static uint16_t lastHz = 0;
static uint8_t ledPattern = 0;
static uint16_t ledT = 0;

static os_timer_t soundTimer;

static void tone_espboy(uint16_t hz) {
    if (hz == lastHz) return;
    lastHz = hz;
    if (!hz || isMuted) {
        myESPboy.noPlayTone();
    } else {
        myESPboy.playTone(hz);
    }
}

static uint16_t noteHz(uint8_t n) {
    static const PROGMEM uint16_t TOP[12] = {4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902};
    int sh = 9 - n / 12;
    return sh >= 0 ? (uint16_t)(pgm_read_word(&TOP[n % 12]) >> sh) : 0;
}

static void scoreTick() {
    if (!scorePos) return;
    if (scoreWait) { scoreWait--; return; }
    for (int guard = 0; guard < 16; guard++) {
        // Добавляем приведение типа: (const uint8_t*)
        uint8_t b = pgm_read_byte((const uint8_t*)scorePos);
        if (b < 0x80) {
            scoreWait = (uint16_t)((b << 8) | pgm_read_byte((const uint8_t*)&scorePos[1]));
            scorePos += 2;
            if (scoreWait) { scoreWait--; return; }
            continue;
        }
        uint8_t cmd = b & 0xF0, ch = b & 3;
        if (cmd == 0x90) { notes[ch] = pgm_read_byte((const uint8_t*)&scorePos[1]); scorePos += 2; }
        else if (cmd == 0x80) { notes[ch] = 0; scorePos += 1; }
        else if (b == 0xE0 && scoreLoops) { scorePos = score; }
        else { scorePos = nullptr; for (auto &n : notes) n = 0; return; }
    }
}

static uint16_t musicHz() {
    if (mode == 2) {
        if (notes[0]) { leadHold = LEAD_HOLD_MS; return noteHz(notes[0]); }
        if (leadHold) { leadHold--; return 0; }
        for (uint8_t k = 1; k < 4; k++) if (notes[k]) return noteHz(notes[k]);
        return 0;
    }
    if (++arpT >= ARP_MS || !notes[arpCh]) {
        arpT = 0;
        for (uint8_t k = 0; k < 4; k++) {
            arpCh = (arpCh + 1) & 3;
            if (notes[arpCh]) break;
        }
    }
    return notes[arpCh] ? noteHz(notes[arpCh]) : 0;
}

static void ICACHE_RAM_ATTR soundTimerCallback(void *pArg) {
    if (!started) return;
    scoreTick();
    const Step *s = (const Step *)fxSteps;
    if (s) {
    
    
   const Step &st = s[fxI];
   uint16_t st_hz = pgm_read_word(&st.hz);
   uint16_t st_endHz = pgm_read_word(&st.endHz);
   uint16_t st_ms = pgm_read_word(&st.ms);

   uint16_t hz = st_hz;
   if (hz && st_endHz) hz = (uint16_t)(st_hz + ((int32_t)st_endHz - st_hz) * fxT / st_ms);
   if (++fxT >= st_ms) {
      fxT = 0;
      if (++fxI >= fxN) { fxSteps = nullptr; fxPrio = 0; }
    }
   tone_espboy(hz);     
    } else {
        tone_espboy(musicHz());
    }
}

namespace audio {

bool begin(uint8_t m) {
    mode = m;
    if (!m) { started = false; tone_espboy(0); return true; }
    if (!started) {
        os_timer_disarm(&soundTimer);
        os_timer_setfn(&soundTimer, soundTimerCallback, NULL);
        os_timer_arm(&soundTimer, 1, true); // Вызов каждые 1 мс
    }
    started = true;
    return true;
}

void setMode(uint8_t m) {
    if (!m) { started = false; lastHz = 1; tone_espboy(0); }
    begin(m);
}

static void play(const Step *st, uint8_t n, uint8_t prio) {
    if (!started || isMuted) return;
    if (fxSteps && prio < fxPrio) return;
    fxSteps = st; fxN = n; fxI = 0; fxT = 0; fxPrio = prio;
}


#define S(hz, end, ms) { (uint16_t)(hz), (uint16_t)(end), (uint16_t)(ms) }
#define REST(ms)       { 0, 0, (uint16_t)(ms) }

static const PROGMEM Step DEAL[]      = { S(3600, 1500, 22) };
static const PROGMEM Step FLIP[]      = { S(2300, 0, 8), REST(5), S(3300, 0, 12) };
static const PROGMEM Step CHIP[]      = { S(3100, 0, 12), REST(9), S(3700, 0, 26) };
static const PROGMEM Step CURSOR[]    = { S(2100, 0, 10) };
static const PROGMEM Step SELECT[]    = { S(1700, 0, 18), S(2600, 0, 30) };
static const PROGMEM Step DENY[]      = { S(900, 650, 70) };
static const PROGMEM Step WIN[]       = { S(2093, 0, 60), S(2637, 0, 60), S(3136, 0, 60), S(4186, 0, 170) };
static const PROGMEM Step BLACKJACK[] = {
    S(1568, 0, 50), S(2093, 0, 50), S(2637, 0, 50), S(3136, 0, 90),
    S(2093, 0, 40), S(2637, 0, 40), S(2093, 0, 40), S(2637, 0, 40),
    S(3136, 0, 40), S(4186, 0, 40), S(3136, 0, 40), S(4186, 0, 40),
    S(2000, 4200, 220) };
static const PROGMEM Step BUST[]      = { S(1600, 950, 110), S(950, 560, 130), S(560, 330, 230) };
static const PROGMEM Step PUSH[]      = { S(1760, 0, 70), REST(40), S(1760, 0, 70) };
static const PROGMEM Step LOSE[]      = { S(1300, 950, 140), S(950, 700, 220) };
static const PROGMEM Step PEEK[]      = { S(1400, 0, 16) };
static const PROGMEM Step SHUFFLE[]   = {
    S(3000, 0, 7), REST(12), S(3400, 0, 7), REST(12), S(3100, 0, 7), REST(12), S(3500, 0, 7), REST(12),
    S(3000, 0, 7), REST(12), S(3400, 0, 7), REST(12), S(3200, 0, 7), REST(12), S(3600, 0, 7), REST(60),
    S(2400, 3800, 120) };
static const PROGMEM Step COIN[]      = { S(2800, 0, 10), S(3700, 0, 28) };
static const PROGMEM Step SPLIT[]     = { S(1500, 3100, 90), REST(20), S(3100, 0, 30) };
static const PROGMEM Step DOUBLE[]    = { S(2000, 0, 30), REST(20), S(2600, 0, 30), REST(20), S(3200, 0, 60) };
static const PROGMEM Step INSURANCE[] = { S(2637, 0, 60), S(3136, 0, 60), S(2637, 0, 60), S(3136, 0, 120) };
static const PROGMEM Step BROKE[]     = { S(1568, 1480, 300), S(1480, 1397, 300), S(1397, 1319, 300), S(1319, 1180, 800) };
static const PROGMEM Step REVEAL[]    = { S(1800, 3000, 60) };
static const PROGMEM Step WHOOSH[]    = { S(1200, 3800, 90) };

struct SfxDef { const Step *steps; uint8_t n, prio; };
#define DEF(a, p) { a, (uint8_t)(sizeof(a) / sizeof(a[0])), p }
static const PROGMEM SfxDef DEFS[(int)Sfx::COUNT] = {
    DEF(DEAL, 1), DEF(FLIP, 1), DEF(CHIP, 1), DEF(CURSOR, 0), DEF(SELECT, 1), DEF(DENY, 1),
    DEF(WIN, 3), DEF(BLACKJACK, 4), DEF(BUST, 3), DEF(PUSH, 3), DEF(LOSE, 3), DEF(PEEK, 1),
    DEF(SHUFFLE, 2), DEF(COIN, 1), DEF(SPLIT, 2), DEF(DOUBLE, 2), DEF(INSURANCE, 3), DEF(BROKE, 4),
    DEF(REVEAL, 2), DEF(WHOOSH, 1),
};

void sfx(Sfx s) {
    const Step* steps = (const Step*)pgm_read_dword(&DEFS[(int)s].steps);
    uint8_t n = pgm_read_byte(&DEFS[(int)s].n);
    uint8_t prio = pgm_read_byte(&DEFS[(int)s].prio);
    play(steps, n, prio);
}

void blip(uint16_t hz, uint16_t ms) {
    if (fxSteps && fxPrio > 1) return;
    blipStep.hz = hz; blipStep.endHz = 0; blipStep.ms = ms;
    play(&blipStep, 1, 0);
}

void music(Song s, bool loop) {
    const uint8_t *data; size_t n;
    music::get((uint8_t)s, loop, data, n);
    score = data; scorePos = data; scoreWait = 0; scoreLoops = true;
    for (auto &x : notes) x = 0;
    leadHold = 0;
}

void loopMusic(bool on) { scoreLoops = on; }
bool musicPlaying() { return started && scorePos; }

void stopMusic() {
    scorePos = nullptr;
    for (auto &x : notes) x = 0;
}

void mute(bool m) { isMuted = m; lastHz = 1; }
bool muted() { return isMuted; }

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
    if (on) myESPboy.myLED.setRGB(0, 100, 0);
    else myESPboy.myLED.setRGB(0, 0, 0);
}

}  // namespace audio