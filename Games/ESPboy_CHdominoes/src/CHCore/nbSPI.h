#pragma once
#include <SPI.h>

#ifdef NBSPI_TDBG_IO
    #if NBSPI_TDBG_IO < 16
        #define NBSPI_TDBG_HIGH GPOS = (1<<NBSPI_TDBG_IO)
        #define NBSPI_TDBG_LOW GPOC = (1<<NBSPI_TDBG_IO)
    #else
        #error "Use IO 0-15!"
    #endif
#else
    #define NBSPI_TDBG_HIGH
    #define NBSPI_TDBG_LOW
#endif

static volatile uint16_t _nbspi_size = 0;
static volatile uint32_t * _nbspi_data;
static volatile boolean _nbspi_isbusy = false;

inline void nbSPI_writeBytes(uint8_t *data, uint16_t size);
inline boolean nbSPI_isBusy();
inline void nbSPI_writeChunk();
inline void nbSPI_ISR();

IRAM_ATTR inline boolean nbSPI_isBusy() {
    if(_nbspi_isbusy) return true;
    return (SPI1CMD & SPIBUSY);
}

IRAM_ATTR inline void nbSPI_writeBytes(uint8_t *data, uint16_t size) {
    NBSPI_TDBG_HIGH;
    _nbspi_isbusy = true;
    _nbspi_size = size;
    _nbspi_data = (uint32_t*) data;
    SPI0S &= ~(0x1F);
    SPI1S &= ~(0x1F);
    ETS_SPI_INTR_ATTACH(nbSPI_ISR, NULL);
    ETS_SPI_INTR_ENABLE();
    nbSPI_writeChunk();
    NBSPI_TDBG_LOW;
}

IRAM_ATTR inline void nbSPI_writeChunk() {    
    uint16_t size = _nbspi_size;
    if(size > 64) size = 64;
    _nbspi_size -= size;
    const uint32_t bits = (size * 8) - 1;
    const uint32_t mask = ~(SPIMMOSI << SPILMOSI);
    SPI1U1 = ((SPI1U1 & mask) | (bits << SPILMOSI));

    uint32_t * fifoPtr = (uint32_t*)&SPI1W0;
    uint8_t dataSize = ((size + 3) / 4);
    while(dataSize--) {
        *fifoPtr = *_nbspi_data;
        _nbspi_data++;
        fifoPtr++;
    }
    __sync_synchronize();

    if(_nbspi_size > 0) {
        SPI1S |= SPISTRIE;
    } else {
        ETS_SPI_INTR_DISABLE();
    }

    SPI1CMD |= SPIBUSY;
    if(_nbspi_size == 0) _nbspi_isbusy = false;
}

IRAM_ATTR inline void nbSPI_ISR() {
    NBSPI_TDBG_HIGH;
    if(SPIIR & (1 << SPII0)) {
        SPI0S &= ~(0x1F);
    }
    if(SPIIR & (1 << SPII1)) {
        SPI1S &= ~(0x1F);
        nbSPI_writeChunk();
    }
    NBSPI_TDBG_LOW;
}