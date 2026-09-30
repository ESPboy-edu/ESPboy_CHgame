#include <Arduino.h>
#include <EEPROM.h>
#include "Save.h"
#include "../game/Round.h"

namespace save {

static const uint32_t MAGIC = 0x4A424843u;
static const uint16_t VERSION = 1;

struct Record {
    uint32_t magic;
    uint16_t version, seq;
    int32_t  purse;
    uint8_t  hasGame, pad[3];
    Options  opt;
    Stats    stats;
    uint32_t crc;
};

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
           r->crc == crc32((const uint8_t *)r, (uint32_t)(sizeof(Record) - 4));
}

bool available() { return true; }

bool load(Round &r, bool &hasGame) {
    hasGame = false;
    Record rec;
    EEPROM.get(0, rec);
    if (!valid(&rec)) return false;
    
    r.opt = rec.opt;
    r.stats = rec.stats;
    hasGame = rec.hasGame && rec.purse > 0;
    if (hasGame) r.purse = rec.purse;
    return true;
}

bool store(const Round &r, bool hasGame) {
    Record rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = MAGIC;
    rec.version = VERSION;
    rec.purse = r.purse;
    rec.hasGame = hasGame ? 1 : 0;
    rec.opt = r.opt;
    rec.stats = r.stats;
    rec.crc = crc32((const uint8_t *)&rec, (uint32_t)(sizeof(rec) - 4));
    
    EEPROM.put(0, rec);
    return EEPROM.commit();
}

}  // namespace save