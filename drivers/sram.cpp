//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifdef ZERO_DRIVERS_SPIMEM


#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/atomic.h>

#include "spi.h"
#include "sram.h"
#include "../core/thread.h"


using namespace zero;


namespace {

    const uint8_t CMD_READ = 3;
    const uint8_t CMD_WRITE = 2;

    auto _spiXferMode = SpiXferMode::Tx;
    uint8_t _dummyTxByte = 0;
    volatile uint8_t* _txCursor = 0UL;
    volatile uint8_t* _rxCursor = 0UL;
    volatile uint32_t _xferBytes = 0ULL;

    Synapse _spiReadySyn;
    SpiMemory* _curController = 0UL;


    void setSpiIsrEnable(const bool en)
    {
        if (en) {
            SPCR |= (1 << SPIE);

        } else {
            SPCR &= ~(1 << SPIE);
        }
    }


    uint8_t spiXfer(const uint8_t c)
    {
        SPDR = c;
        while (!(SPSR & (1 << SPIF)));
        return SPDR;
    }


}


// ctor
SpiMemory::SpiMemory(
    const uint32_t capacityBytes,               // how many bytes does the chip hold?
    volatile uint8_t* csDdr,                    // DDR for the CS for the chip
    volatile uint8_t* csPort,                   // PORT for CS
    const uint8_t csPin,                        // pin number for CS
    Synapse readySyn)                           // Synapse to fire when ready to transfer
{
    _capacityBytes = capacityBytes;
    _csDdr = csDdr;
    _csPort = csPort;
    _csPinMask = 1 << csPin;

    // setup the SPI GPIO
    SPI_DDR |= (SCLK | MOSI);
    SPI_DDR &= ~MISO;

    // chip select for this chip
    *_csDdr |= _csPinMask;

    // make sure ISRs for SPI are off
    setSpiIsrEnable(false);

    // full-speed MASTER mode SPI, kkplzthx
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR |= (1 << SPI2X);

    // signal the Synapse that we're ready to go
    _spiReadySyn = readySyn;
    _spiReadySyn.signal();
}


// dtor
SpiMemory::~SpiMemory()
{
    // stop interrupting me!
    setSpiIsrEnable(false);

    // switch off the SPI hardware
    SPCR = 0;
    SPSR = 0;

    // return the CS line to floating
    *_csPort &= ~_csPinMask;
    *_csDdr &= ~_csPinMask;

    // clear signals and forget
    _spiReadySyn.clearSignals();
    _spiReadySyn.clear();
}


void SpiMemory::select()
{
    *_csPort &= ~_csPinMask;
}


void SpiMemory::deselect()
{
    *_csPort |= _csPinMask;
}


void SpiMemory::read(void* dest, const uint32_t srcAddr, const uint32_t numBytes)
{
    // wait until there's no controller using the SPI
    while (_curController);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // tell the ISR who we are
        _curController = this;

        // make sure no-one falls through while we're working
        _spiReadySyn.clearSignals();

        // set up the housekeeping
        _txCursor = 0UL;                        // we are reading data, so no TX buffer
        _rxCursor = (uint8_t*) dest;
        _xferBytes = numBytes;
        _spiXferMode = SpiXferMode::Rx;        

        // tell the SPI chip that we want to play a game
        select();

        // tell it that we want to read data starting at SPI-SRAM address of srcAddress
        sendReadCommand(srcAddr);

        // enable the ISR
        setSpiIsrEnable(true);

        // push the first one out to kickstart it
        SPDR = _dummyTxByte;
    }
}


void SpiMemory::write(const void* src, const uint32_t destAddress, const uint32_t numBytes)
{
    // wait until there's no controller using the SPI
    while (_curController);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // tell the ISR who we are
        _curController = this;

        // make sure no-one falls through while we're working
        _spiReadySyn.clearSignals();

        // set up the housekeeping
        _rxCursor = 0UL;
        _txCursor = (uint8_t*) src;
        _xferBytes = numBytes;
        _spiXferMode = SpiXferMode::Tx;

        // tell the SPI chip that we want to play a game
        select();

        // tell it that we want to read data starting at SPI-SRAM address of srcAddress
        sendWriteCommand(destAddress);

        // enable the ISR
        setSpiIsrEnable(true);

        // push the first one out to kickstart it
        const uint8_t firstByte = *_txCursor++;

        SPDR = firstByte;
    }
}


ISR(SPI_STC_vect)
{
    uint8_t rxByte = 0;
    bool storeRxByte = false;

    // capture the input
    if (_spiXferMode == SpiXferMode::Rx) {
        rxByte = SPDR;
        storeRxByte = true;
    }

    // another 1 bytes the dust
    _xferBytes--;

    if (_xferBytes) {

        // send the correct output
        if (_spiXferMode == SpiXferMode::Tx) {
            SPDR = *_txCursor++;

        } else {
            SPDR = _dummyTxByte;
        }
    }

    if (storeRxByte) {
        *_rxCursor++ = rxByte;
    }

    if (!_xferBytes) {
        setSpiIsrEnable(false);

        // transfer complete
        _spiReadySyn.signal();
        _curController->deselect();
        _curController = 0UL;
    }

}


void SpiMemory::sendAddress(const uint32_t addr)
{
    if (_capacityBytes > 16777216ULL) {
        spiXfer(addr >> 24);
    }

    if (_capacityBytes > 65536ULL) {
        spiXfer(addr >> 16);
    }

    spiXfer(addr >>  8);
    spiXfer(addr >>  0);
}


void SpiMemory::sendReadCommand(const uint32_t addr)
{
    spiXfer(CMD_READ);
    sendAddress(addr);
}


void SpiMemory::sendWriteCommand(const uint32_t addr)
{
    spiXfer(CMD_WRITE);
    sendAddress(addr);
}


#endif
