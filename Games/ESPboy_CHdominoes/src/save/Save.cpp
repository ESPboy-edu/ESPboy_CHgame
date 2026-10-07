#pragma GCC optimize("Os", "no-ipa-sra")
#include <Arduino.h>
#include <string.h>
#include <stddef.h>
#include "../../config.h"
#include "../CHCore/CHGfx.h"
#include "../CHCore/RamFunc.h"
#include "Save.h"

static const uint32_t MAGIC = 0x4D444843u;       // "CHDM"
static const uint8_t VERSION = 1;

struct Record {
    uint32_t magic;
    uint8_t  version, hasGame;
    uint16_t seq;
    Options  opt;
    Stats    stats;
    match::Record game;
    uint32_t crc;
};

static const uint32_t CRC_OVER = offsetof(Record, crc);

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

#include "../CHCore/ESPboy_SaveCore.hpp"