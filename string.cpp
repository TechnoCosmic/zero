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


int isdigit(const char c) {
	return '0' <= c && c <= '9';
}


int isalpha(const char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}


int isapnum(const char c) {
	return isdigit(c) || isalpha(c);
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

	while (charA && *b) {
		if (charA != *b) {
			return charA - *b;
		}

		a++;
		charA = pgm_read_byte_near(a);
		b++;
	}

	return charA - *b;
}


int strcmppgm_wc(const char* a, char *b) {
	char charA = pgm_read_byte_near(a);

	while (charA && *b) {
		if (*b == '*') {
			return 0;
		}

		if (*b != '?') {
			if (charA != *b) {
				return charA - *b;
			}
		}

		a++;
		charA = pgm_read_byte_near(a);
		b++;
	}

	return charA - *b;
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


char* itoa(int32_t num, char* str, const uint16_t base, const bool positive, const bool ucase) {
	int i = 0;
	bool isNegative = false;

	// handle 0 explicitely, otherwise empty string is printed for 0
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		
		return str;
	}
	// in standard itoa(), negative numbers are handled only with
	// base 10. otherwise numbers are considered unsigned.
	if (positive && num < 0 && base == 10) {
		isNegative = true;
		num = -num;
	}

	// process individual digits
	while (num != 0) {
		int rem = num % base;

		if (ucase) {
			str[i] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';

		} else {
			str[i] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		}

		i++;
		num /= base;
	}
	
	// if number is negative, append '-'
	if (isNegative) {
		str[i++] = '-';
	}
	
	// append null terminator
	str[i] = '\0';
	
	// reverse the string
	reverse(str, i);

	// escape
	return str;
}


// Return true if str matches pattern, false otherwise
bool matches(const char* str, char* pattern, const bool srcIsFlash) {
    bool rc = true;
	char* st = (char*) str;
	char* pt = (char*) pattern;
    char* pAfterLastWild = 0UL;     // The location after the last '*', if we’ve encountered one
    char* pAfterLastTame = 0UL;     // The location in the tame string, from which we started after last wildcard
    char t, w;

    while (true) {
		if (srcIsFlash) {
			t = pgm_read_byte(st);
		} else {
			t = *st;
		}

		w = *pt;

        if (!t) {

            if (!w) {
                break;

            } else if (w == '*') {
                pt++;
                continue;

            } else if (pAfterLastTame) {
                if (!*pAfterLastTame) {
                    rc = false;
                    break;
                }
                st = pAfterLastTame++;
                pt = pAfterLastWild;
                continue;
            }

            rc = false;
            break;

        } else {
            if (t != w) {
                if (w == '*') {
                    pAfterLastWild = ++pt;
                    pAfterLastTame = st;
                    w = *pt;

                    if (!w) {
                        break;
                    }
                    continue;

                } else if (w == '?') {
                    // do nothing - the characters are considered a match

                } else if (pAfterLastWild) {
                    if (pAfterLastWild != pt) {
                        pt = pAfterLastWild;
                        w = *pt;
                        
                        if (t == w) {
                            pt++;
                        }
                    }

                    st++;
                    continue;

                } else {
                    rc = false;
                    break;
                }
            }
        }

        st++;
        pt++;
    }

    return rc;
}
