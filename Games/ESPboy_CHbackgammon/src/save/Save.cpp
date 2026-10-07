#pragma GCC optimize("Os", "no-ipa-sra")   // cold code: size over speed
#include <Arduino.h>
#include <string.h>
#include <stddef.h>
#include "../CHCore/CHGfx.h"
#include "../../config.h"
#include "../CHCore/RamFunc.h"
#include "Save.h"

#if CHBG_LEAN
// A build without saving (config.h).
namespace save {
bool available() { return false; }
bool load(Options &, Stats &, bool &hasGame) { hasGame = false; return false; }
bool loadGame() { return false; }
bool store(const Options &, const Stats &, bool) { return false; }
} // namespace save
#else

namespace save {
static const uint32_t MAGIC = 0x47424843u;       // "CHBG"
static const uint8_t VERSION = 2;              // 2: matches, the cube, more options
static const uint32_t PAGE = 256;

struct Record {
    uint32_t magic;
    uint8_t  version, hasGame;
    uint16_t seq;
    Options  opt;
    Stats    stats;
    match::Record game;
    uint32_t crc;
};

static_assert(sizeof(Record) <= PAGE, "save record must fit one flash page");
static const uint32_t CRC_OVER = offsetof(Record, crc);

static uint16_t lastSeq = 0;
static bool broken = false;

static uint32_t crc32(const uint8_t *p, uint32_t n) {
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

static bool valid(const Record *r) {
    return r->magic == MAGIC && r->version == VERSION &&
           r->crc == crc32((const uint8_t *)r, CRC_OVER);
}
} // ВАЖНО: Закрываем namespace save ЗДЕСЬ!

// Подключаем ядро ESPboy (оно само откроет свой namespace save для функций load и store)
#include "../CHCore/ESPboy_SaveCore.hpp"

#endif