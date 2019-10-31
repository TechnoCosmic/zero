/*
 * zero - pre-emptive multitasking kernel for AVR
 *
 *  Techno Cosmic Research Institute	Dirk Mahoney			dirk@tcri.com.au
 *  Catchpole Robotics					Christian Catchpole		christian@catchpole.net
 * 
 */

#include <avr/pgmspace.h>

int islower(const char c) {
	return c >= 'a' && c <= 'z';
}

int isupper(const char c) {
	return c >= 'A' && c <= 'Z';
}

char tolower(const char c) {
	if (isupper(c)) {
		return c + 32;
	}
	return c;
}

char toupper(const char c) {
	if (islower(c)) {
		return c - 32;
	}
	return c;
}

void* memset(uint8_t* ptr, const uint8_t d, const uint16_t sz) {
	for (uint16_t i = 0; i < sz; i++) {
		ptr[i] = d;
	}
	return ptr;
}

void* memcpy(uint8_t* dest, const uint8_t* src, const uint16_t count) {
	uint16_t c = count;

	while (c--) {
		dest[count] = src[count];
	}
	return dest;
}

int strlen(const char* s) {
	int rc = 0;

	while (*s++) {
		rc++;
	}

	return rc;
}

int strlenpgm(const char* s) {
	int rc = 0;
	char charA = pgm_read_byte_near(s);

	while (charA) {
		rc++;
		charA = pgm_read_byte_near(++s);
	}

	return rc;
}

int strcmppgm(const char* a, char *b) {
	char charA = pgm_read_byte_near(a);

	while (charA) {
		if (charA != *b) {
			return charA - *b;
		}
		a++;
		charA = pgm_read_byte_near(a);
		b++;
	}
	return 0;
}

// swap two characters around
void swap(char* a, char* b) {
	char t = *a;
	*a = *b;
	*b = t;
}

// flip a string backwards
void reverse(char* str, int length) {
	int start = 0;
	int end = length - 1;

	while (start < end) {
		swap(&str[start], &str[end]);
		start++;
		end--;
	}
}

// Implementation of itoa()
char* itoa(int16_t num, char* buffer, const int16_t base, const bool positive) {
	uint8_t i = 0;
	bool isNegative = false;

	// handle 0 explicitely, otherwise empty string is printed for 0
	if (num == 0) {
		buffer[i++] = '0';
		buffer[i] = '\0';
		return buffer;
	}

	// in standard itoa(), negative numbers are handled only with
	// base 10. otherwise numbers are considered unsigned.
	if (positive && num < 0 && base == 10) {
		isNegative = true;
		num = -num;
	}

	// process individual digits
	while (num != 0) {
		int32_t rem = num % base;

		buffer[i] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
		i++;
		num /= base;
	}

	// if number is negative, append '-'
	if (isNegative) {
		buffer[i++] = '-';
	}

	// append null terminator
	buffer[i] = '\0';

	// reverse the string
	reverse(buffer, i);

	// escape
	return buffer;
}
