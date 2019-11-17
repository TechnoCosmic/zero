/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#ifndef TCRI_ZERO_STRING_H
#define TCRI_ZERO_STRING_H

int islower(const char c);
int isupper(const char c);
int isspace(const char c);
int isdigit(const char c);
int isalpha(const char c);
int inalnum(const char c);

char tolower(const char c);
char toupper(const char c);
void* memset(uint8_t* dest, const uint8_t value, const uint16_t count);
void* memcpy(uint8_t* dest, const uint8_t* src, const uint16_t count);

int strlen(const char*);
int strlenpgm(const char*);
int strcmppgm(const char*, char*);
bool matches(const char* str, char* pattern, const bool srcIsFlash);

char* itoa(int32_t num, char* buffer, const uint16_t base, const bool positive, const bool ucase);

#endif