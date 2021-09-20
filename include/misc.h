#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>

int stringCompare(char* str1, char* str2);

int stringLength(char* str);

//str must first be validated by isStringInteger, not safe
int stringToInteger(char* str, int* isOverflow);

int isStringInteger(char* str);

int getDTMFBySymbol(int x, int* Fr, int* Fc);

//symbol is not validated in this function, it's done by getDTMFBySymbol;
int getNextDTMFEvent(FILE *events_in, uint32_t* start, uint32_t* end, int* symbol);

int writeNextDTMFEvent(FILE *events_out, uint32_t start, uint32_t end, int symbol);

int getStrongestFrequencyIndexes(double r0, double r1, double r2, double r3, double r4, double r5, double r6, double r7, int* IFr, int* IFc);

#endif
