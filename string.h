/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_STRING_H
#define TCRI_ZERO_STRING_H

// VT100 color codes
#define RED "\e[31m"
#define GREEN "\e[32m"
#define BLUE "\e[34m"
#define WHITE "\e[37m"

int islower(const char c);
int isupper(const char c);

char tolower(const char c);
char toupper(const char c);
void* memset(uint8_t* dest, const uint8_t value, const uint16_t count);
void* memcpy(uint8_t* dest, const uint8_t* src, const uint16_t count);

int strcmppgm(const char*, char*);

char* itoa(int16_t num, char* buffer, const int16_t base, const bool positive);

#endif