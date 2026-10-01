#pragma GCC optimize("Os")
#include <Arduino.h>
#include <EEPROM.h>
#include <string.h>
#include "../../config.h"
#include "Save.h"

namespace save {

#if CHCH_LEAN
// Если включен режим LEAN, сохранения отключаются
bool available() { return false; }
bool load(Options &, Stats &, bool &hasGame) { hasGame = false; return false; }
bool loadGame() { return false; }
bool store(const Options &, const Stats &, bool) { return false; }
#else

static const uint32_t MAGIC = 0x53434843u;       // "CHCS"
static const uint8_t VERSION = 2;                // 2: three opponents

// Структура для сохранения (размер около 232 байт)
struct Record {
    uint32_t magic;
    uint8_t  version, hasGame;
    uint16_t seq;
    Options  opt;
    Stats    stats;
    match::Record game;
    uint32_t crc;
};

static bool eeprom_initialized = false;
static Record currentRecord;

// Оригинальная функция проверки целостности от автора игры
static uint32_t crc32(const uint8_t *p, uint32_t n) {
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

// Инициализация сектора EEPROM в Flash-памяти ESP8266 (выделяем 512 байт с запасом)
static void initEEPROM() {
    if (!eeprom_initialized) {
        EEPROM.begin(512); 
        eeprom_initialized = true;
    }
}

static bool valid(const Record *r) {
    return r->magic == MAGIC && r->version == VERSION &&
           r->crc == crc32((const uint8_t *)r, (uint32_t)(sizeof(Record) - 4));
}

bool available() { 
    return true; 
}

bool load(Options &o, Stats &s, bool &hasGame) {
    initEEPROM();
    hasGame = false;
    
    // Читаем данные из EEPROM начиная с адреса 0
    EEPROM.get(0, currentRecord);
    
    if (!valid(&currentRecord)) {
        return false; // Сохранений еще нет или они повреждены
    }
    
    o = currentRecord.opt;
    s = currentRecord.stats;
    hasGame = currentRecord.hasGame != 0;
    return true;
}

bool loadGame() {
    initEEPROM();
    EEPROM.get(0, currentRecord);
    if (valid(&currentRecord) && currentRecord.hasGame) {
        return match::load(currentRecord.game);
    }
    return false;
}

bool store(const Options &o, const Stats &s, bool withGame) {
    initEEPROM();
    
    currentRecord.magic = MAGIC;
    currentRecord.version = VERSION;
    currentRecord.seq++; // Просто увеличиваем счетчик
    currentRecord.opt = o;
    currentRecord.stats = s;
    currentRecord.hasGame = withGame ? 1 : 0;
    
    if (withGame) {
        match::save(currentRecord.game);
    }
    
    // Считаем контрольную сумму
    currentRecord.crc = crc32((const uint8_t *)&currentRecord, (uint32_t)(sizeof(Record) - 4));
    
    // Записываем структуру и вызываем commit() для физического сохранения во Flash
    EEPROM.put(0, currentRecord);
    bool success = EEPROM.commit();
    
    return success;
}

#endif

}  // namespace save