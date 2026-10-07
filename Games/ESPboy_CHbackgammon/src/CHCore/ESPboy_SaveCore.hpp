// ESPboy_SaveCore.hpp
// Подключается В КОНЦЕ файла Save.cpp оригинальной игры.

#include "ArduboyFX.h" // Локальное подключение (в той же папке)

#ifndef CRC_OVER
#define CRC_OVER (sizeof(Record) - 4)
#endif


namespace save {
#if CHDM_LEAN
bool available() { return false; }
bool load(Options &o, Stats &s, bool &hasGame) { hasGame = false; return false; }
bool loadGame() { return false; }
bool store(const Options &o, const Stats &s, bool withGame) { return false; }
#else
static Record currentRecord;

bool available() { return true; }

bool load(Options &o, Stats &s, bool &hasGame) {
    FX::begin(); 
    hasGame = false;
    if (FX::loadGameState(currentRecord)) {
        if (!valid(&currentRecord)) return false;
        o = currentRecord.opt;
        s = currentRecord.stats;
        hasGame = currentRecord.hasGame != 0;
        return true;
    }
    return false;
}

bool loadGame() {
    FX::begin();
    if (FX::loadGameState(currentRecord)) {
        if (valid(&currentRecord) && currentRecord.hasGame) {
            return match::load(currentRecord.game);
        }
    }
    return false;
}

bool store(const Options &o, const Stats &s, bool withGame) {
    FX::begin();
    currentRecord.magic = MAGIC;
    currentRecord.version = VERSION;
    currentRecord.seq++;
    currentRecord.opt = o;
    currentRecord.stats = s;
    currentRecord.hasGame = withGame ? 1 : 0;
    
    if (withGame) match::save(currentRecord.game);
    currentRecord.crc = crc32((const uint8_t *)&currentRecord, CRC_OVER); 
    
    FX::saveGameState(currentRecord);
    return true;
}
#endif
}