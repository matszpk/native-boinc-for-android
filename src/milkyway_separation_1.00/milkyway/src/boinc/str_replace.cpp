/*
 * str_replace.cpp - extra function from boinc-7.0 library
 * Mateusz Szpakowski
 */

#include <string.h>
#include <stdlib.h>
#include "boinc/str_replace.h"


void strcpy_overlap(char* p, const char* q) {
    while (1) {
        *p++ = *q;
        if (!*q) break;
        q++;
    }
}
