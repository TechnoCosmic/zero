/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_UTIL_H
#define TCRI_ZERO_UTIL_H

#define TOT(a,b) (a)?(a):(b)
#define TTT(a,b) (a)?(b):(0UL)
#define MIN(a,b) (a)<(b)?(a):(b)
#define MAX(a,b) (a)>(b)?(a):(b)

#define ROUND_UP(v,r) ((((v)-1)|((r)-1))+1)

#endif