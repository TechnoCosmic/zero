//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_ATTRS_H
#define TCRI_ZERO_ATTRS_H


#define WEAK __attribute__( ( weak ) )
#define HOT __attribute__( ( hot ) )
#define COLD __attribute__( ( cold ) )
#define NAKED __attribute__( ( naked ) )
#define INLINE __attribute__( ( always_inline ) )
#define CTOR __attribute__( ( constructor ) )
#define DTOR __attribute__( ( destructor ) )
#define ALIGNED( x ) __attribute__( ( aligned( ( x ) ) ) )
#define CLEANUP( x ) __attribute__( ( cleanup( x ) ) )


#endif