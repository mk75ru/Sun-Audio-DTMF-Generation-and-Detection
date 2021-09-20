#include "misc.h"
#include "debug.h"

#include "dtmf.h"
#include "const.h"

int stringCompare(char* str1, char* str2) {
	int diff = 0;

	do {
		if ((diff = *str1 - *str2) != 0) break;
		str1++;
		str2++;
	} while ( (*str1) != '\0' || (*str2) != '\0');

	return diff;
}

int stringLength(char* str) {
	int count = 0;
	while ((*str) != '\0') {
		str++;
		count++;
	}
	return count;
}

int stringToInteger(char* str, int* isOverflow) { //str must be validated by isStringInteger, not safe
	int num = 0;
	int multiplier = 1;
	while ((*str) != '\0') {
		if ((*str) == 43) {
			str++;
			continue;
		} else if ((*str) == 45) {
			multiplier = -1;
			str++;
			continue;
		}
		if (num > 2147483647 / 10) {
			*isOverflow = 1;
			if (multiplier == -1 && num <= 2147483648 / 10) {
				*isOverflow = 0;
			} else {
				return -1;
			}
		}
		int oprand1 = num * 10;
		int oprand2 = ((*str) - 48);
		if (oprand1 > 2147483647 - oprand2) {
			*isOverflow = 1;
			if (multiplier == -1 && oprand1 <= 2147483648 - oprand2) {
				*isOverflow = 0;
			} else {
				return -1;
			}
		}

		//num = num * 10 + ((*str) - 48) ;
		num = oprand1 + oprand2;

		str++;
	}
	return num * multiplier;
}

int isStringInteger(char* str) {
	if ((*str) == '\0') return 0;
	int signCheck = 0, sizeCheck = 0;
	while ((*str) != '\0') { //not end of the string
		if ((*str) < 48 || (*str) > 57) { //if not digit
			if (!signCheck) { //if not signchecked
				signCheck = 1; //check it
				if ((*str) == 43 || (*str) == 45) {
					str++;
					continue; //make an exception this time
				}
			}
			return 0; //no more exception
		}
		if (signCheck && !sizeCheck) {
			sizeCheck = 1; //check not just one sign only
			if ((*str) == 48 && *(str + 1) == '\0') return 0;
		}
		str++;//goto next char
	}
	return ((!signCheck && !sizeCheck) || (signCheck && sizeCheck)) ? 1 : 0;
}

int getDTMFBySymbol(int x, int* Fr, int* Fc) {
	for (int i = 0; i < NUM_DTMF_ROW_FREQS; i++) {
		for (int j = 0; j < NUM_DTMF_COL_FREQS; j++) {
			int symbol = *(*(dtmf_symbol_names + i) + j);
			if (symbol == x) {
				*Fr = *(dtmf_freqs + i);
				*Fc = *(dtmf_freqs + NUM_DTMF_ROW_FREQS + j);
				return 0;
			}
		}
	}
	return -1;
}

int getNextDTMFEvent(FILE *events_in, uint32_t* start, uint32_t* end, int* symbol) {
	if (events_in == NULL) return EOF;
	int count = 0;
	uint32_t c, fieldValue = 0;
	uint32_t initStart = *start, initEnd = *end;
	while ((c = fgetc(events_in)) != EOF) {
		if (c == 9) {
			switch (count) {
			case 0:
				if ((initStart != 0 || initEnd != 0)  && initEnd > fieldValue) return EOF;
				*start = fieldValue;
				break;
			case 1:
				if ((initStart != 0 || initEnd != 0) && (*start) > fieldValue) return EOF;
				*end = fieldValue;
				break;
			default:
				return EOF;
			}
			fieldValue = 0;
			count++;
		} else if (count == 3 && c == 10) {
			*symbol = fieldValue;
			if ((*start) == (*end)) {
				count = 0;
				fieldValue = 0;
				initStart = *start;
				initEnd = *end;
				fprintf(stderr, "\033[1;33m" "WARNING: %s:%s:%d: %s\n" , \
				        __FILE__,  __FUNCTION__, __LINE__, "Empty Interval Detected");
				continue;
			}
			return ((*start) < (*end)) ? 0 : EOF;
		} else {
			if (count < 2) {
				if (c < 48 || c > 57) return EOF;
				if (fieldValue > UINT32_MAX / 10) return EOF;
				uint32_t oprand1 = fieldValue * 10;
				uint32_t oprand2 = c - 48;
				if (oprand1 > UINT32_MAX - oprand2) return EOF;
				fieldValue = oprand1 + oprand2;
			} else {
				if (count > 3) return EOF;
				fieldValue = c;
				count++;
			}
		}
	}
	if (count == 3) {
		*symbol = fieldValue;
		fprintf(stderr, "\033[1;33m" "WARNING: %s:%s:%d: %s\n" , \
		        __FILE__,  __FUNCTION__, __LINE__, "\\n is not detected as the last character in the last line");
		return ((*start) < (*end)) ? 0 : EOF;
	}
	return EOF;
}

int writeNextDTMFEvent(FILE *events_out, uint32_t start, uint32_t end, int symbol) {
	if (events_out == NULL) return EOF;
	if (fprintf(events_out, "%u\t%u\t%c\n", start, end, symbol) < 0) return EOF;
	return 0;
}

int getStrongestFrequencyIndexes(double r0, double r1, double r2, double r3, double r4, double r5, double r6, double r7, int* IFr, int* IFc) {
	int ciFr = 0, ciFc = 0; //current Strongest Frequency Indexes
	double strongestFr = r0, strongestFc = r4;
	if (r1 > r0) {
		strongestFr = r1;
		ciFr = 1;
		if (r2 > r1) {
			strongestFr = r2;
			ciFr = 2;
			if (r3 > r2) {
				strongestFr = r3;
				ciFr = 3;
			}
		} else if (r3 > r1) {
			strongestFr = r3;
			ciFr = 3;
		}
	} else if (r2 > r0) {
		strongestFr = r2;
		ciFr = 2;
		if (r3 > r2) {
			strongestFr = r3;
			ciFr = 3;
		}
	} else if (r3 > r0) {
		strongestFr = r3;
		ciFr = 3;
	}
	if (strongestFr != r0 && ((strongestFr / r0) < SIX_DB)) return -1;
	if (strongestFr != r1 && ((strongestFr / r1) < SIX_DB)) return -1;
	if (strongestFr != r2 && ((strongestFr / r2) < SIX_DB)) return -1;
	if (strongestFr != r3 && ((strongestFr / r3) < SIX_DB)) return -1;

	if (r5 > r4) {
		strongestFc = r5;
		ciFc = 1;
		if (r6 > r5) {
			strongestFc = r6;
			ciFc = 2;
			if (r7 > r6) {
				strongestFc = r7;
				ciFc = 3;
			}
		} else if (r7 > r5) {
			strongestFc = r7;
			ciFc = 3;
		}
	} else if (r6 > r4) {
		strongestFc = r6;
		ciFc = 2;
		if (r7 > r6) {
			strongestFc = r7;
			ciFc = 3;
		}
	} else if (r7 > r4) {
		strongestFc = r7;
		ciFc = 3;
	}
	if (strongestFc != r4 && ((strongestFc / r4) < SIX_DB)) return -1;
	if (strongestFc != r5 && ((strongestFc / r5) < SIX_DB)) return -1;
	if (strongestFc != r6 && ((strongestFc / r6) < SIX_DB)) return -1;
	if (strongestFc != r7 && ((strongestFc / r7) < SIX_DB)) return -1;
	if ((strongestFr + strongestFc) < MINUS_20DB) return -1;
	double FRatio = strongestFr / strongestFc;
	if ((FRatio < (1 / FOUR_DB)) || (FRatio > FOUR_DB)) return -1;

	*IFr = ciFr;
	*IFc = ciFc;

	return 0;
}