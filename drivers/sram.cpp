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
#include "thread.h"
#include "resource.h"


using namespace zero;


namespace {

    enum class SpiXferMode {
        Tx,
        Rx,
    };


    const uint8_t CMD_READ{ 3 };
    const uint8_t CMD_WRITE{ 2 };

    auto _spiXferMode{ SpiXferMode::Tx };
    volatile uint8_t* _txCursor{ nullptr };
    volatile uint8_t* _rxCursor{ nullptr };
    volatile uint32_t _xferBytes{ 0UL };

    const Synapse* _spiReadySyn{ nullptr };
    SpiMemory* _curController{ nullptr };


    // switches the SPI transfer-complete ISR on and off
    void setSpiIsrEnable( const bool en )
    {
        if ( en ) {
            SPCR |= ( 1 << SPIE );
        }
        else {
            SPCR &= ~( 1 << SPIE );
        }
    }


    // busy-poll exchanges one byte over SPI
    uint8_t spiXfer( const uint8_t c )
    {
        SPDR = c;

        while ( !( SPSR & ( 1 << SPIF ) ) ) {
            // empty
        }

        return SPDR;
    }

}    // namespace


// ctor
SpiMemory::SpiMemory(
    const uint32_t capacityBytes,                       // how many bytes does the chip hold?
    const Gpio& chipSelect,                             // Gpio object for the CS line
    const Synapse& readySyn )                           // Synapse to fire when ready to transfer
:
    _capacityBytes( capacityBytes )
{
    if ( !resource::obtain( resource::ResourceId::Spi ) ) {
        return;
    }

    _chipSelectPin = &chipSelect;
    _chipSelectPin->setAsOutput();

    // setup the SPI GPIO
    SPI_DDR |= ( SCLK | MOSI );
    SPI_DDR &= ~MISO;

    // make sure it's not selected
    deselect();

    // make sure ISRs for SPI are off
    setSpiIsrEnable( false );

    // full-speed MASTER mode SPI, kkplzthx
    SPCR = ( 1 << SPE ) | ( 1 << MSTR );
    SPSR |= ( 1 << SPI2X );

    // signal the Synapse that we're ready to go
    _spiReadySyn = &readySyn;
    _spiReadySyn->signal();
}


// dtor
SpiMemory::~SpiMemory()
{
    if ( *this ) {
        // stop interrupting me!
        setSpiIsrEnable( false );

        // switch off the SPI hardware
        SPCR = 0;
        SPSR = 0;

        // return the CS line to floating
        if ( _chipSelectPin ) {
            _chipSelectPin->reset();
        }


        // clear signals and forget
        if ( _spiReadySyn ) {
            _spiReadySyn->clearSignals();
            _spiReadySyn = nullptr;
        }

        // free the resource
        resource::release( resource::ResourceId::Spi );
    }
}


// validity checking
SpiMemory::operator bool() const
{
    return _chipSelectPin;
}


// select the chip, by pulling CS low
void SpiMemory::select() const
{
    _chipSelectPin->switchOff();
}


// deselect the chip, by pulling CS high
void SpiMemory::deselect() const
{
    _chipSelectPin->switchOn();
}


// Reads data from the external memory into the local SRAM
void SpiMemory::read(
    void* dest,                                         // destination address, in local SRAM
    const uint32_t srcAddr,                             // source address for the data, in external SPI memory
    const uint32_t numBytes )                           // number of bytes to read
{
    // wait until there's no controller using the SPI
    while ( _curController ) {
        // empty
    }

    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // tell the ISR who we are
        _curController = this;

        // make sure no-one falls through while we're working
        if ( _spiReadySyn ) {
            _spiReadySyn->clearSignals();
        }

        // set up the housekeeping
        _txCursor = nullptr;                            // we are reading data, so no TX buffer
        _rxCursor = (uint8_t*) dest;
        _xferBytes = numBytes;
        _spiXferMode = SpiXferMode::Rx;

        // tell the SPI chip that we want to talk to it
        select();

        // tell it that we want to read data from srcAddress
        sendReadCommand( srcAddr );

        // enable the ISR
        setSpiIsrEnable( true );

        // push the first one out to kickstart it
        SPDR = 0;
    }
}


// Writes data from the local SRAM to the external memory chip
void SpiMemory::write(
    const void* src,                                    // source data address, in local SRAM
    const uint32_t destAddress,                         // destination address, in external SPI memory
    const uint32_t numBytes )                           // number of the bytes to write
{
    // wait until there's no controller using the SPI
    while ( _curController ) {
        // empty
    }

    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        // tell the ISR who we are
        _curController = this;

        // make sure no-one falls through while we're working
        if ( _spiReadySyn ) {
            _spiReadySyn->clearSignals();
        }

        // set up the housekeeping
        _rxCursor = nullptr;                            // writing means no RX buffer
        _txCursor = (uint8_t*) src;
        _xferBytes = numBytes;
        _spiXferMode = SpiXferMode::Tx;

        // tell the SPI chip that we want to talk to it
        select();

        // tell it that we want to write data to destAddress
        sendWriteCommand( destAddress );

        // enable the ISR
        setSpiIsrEnable( true );

        // push the first one out to kickstart it
        SPDR = *_txCursor++;
    }
}


// This ISR is run whenever the SPI hardware finishes exchanging a single byte
ISR( SPI_STC_vect )
{
    uint8_t rxByte{ 0 };
    bool storeRxByte{ false };

    // capture the input
    if ( _spiXferMode == SpiXferMode::Rx ) {
        rxByte = SPDR;
        storeRxByte = true;
    }

    // another 1 bytes the dust
    _xferBytes--;

    // if there's more data to transfer...
    if ( _xferBytes ) {
        // send the correct output
        if ( _spiXferMode == SpiXferMode::Tx ) {
            SPDR = *_txCursor++;
        }
        else {
            SPDR = 0;
        }
    }

    // remember the received byte
    if ( storeRxByte ) {
        *_rxCursor++ = rxByte;
    }

    // disable things if we're done
    if ( !_xferBytes ) {
        setSpiIsrEnable( false );

        if ( _spiReadySyn ) {
            _spiReadySyn->signal();
        }

        _curController->deselect();
        _curController = nullptr;
    }
}


// sends an address to the memory chip
void SpiMemory::sendAddress( const uint32_t addr ) const
{
    if ( _capacityBytes > ( 1ULL << 24 ) ) {
        spiXfer( addr >> 24 );
    }

    if ( _capacityBytes > ( 1ULL << 16 ) ) {
        spiXfer( addr >> 16 );
    }

    spiXfer( addr >> 8 );
    spiXfer( addr >> 0 );
}


// sends a CMD_READ to the memory chip
void SpiMemory::sendReadCommand( const uint32_t addr ) const
{
    spiXfer( CMD_READ );
    sendAddress( addr );
}


// sends a CMD_WRITE to the memory chip
void SpiMemory::sendWriteCommand( const uint32_t addr ) const
{
    spiXfer( CMD_WRITE );
    sendAddress( addr );
}


#endif
