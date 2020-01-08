//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_SPI_H
#define TCRI_ZERO_SPI_H


#if SPI_CFG == 1
    #define SPI_PORT    PORTB
    #define SPI_PIN     PINB
    #define SPI_DDR     DDRB
    #define MOSI        (1 << PINB3)
    #define MISO        (1 << PINB4)
    #define SCLK        (1 << PINB5)

#elif SPI_CFG == 2
    #define SPI_PORT    PORTB
    #define SPI_PIN     PINB
    #define SPI_DDR     DDRB
    #define MOSI        (1 << PINB5)
    #define MISO        (1 << PINB6)
    #define SCLK        (1 << PINB7)
#endif


#endif