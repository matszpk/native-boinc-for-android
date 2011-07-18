#ifndef GLOBAL_H
#define GLOBAL_H

#define H 0
#define M3 1
#define M4 2

#define NLINE1 76
#define NLINE2 58
#define CT 250
#define NCT CT+3
#define TICKET 21
#define NTICKET 34
#define NDIALOG 18

#ifndef DBL_EPSILON
#define DBL_EPSILON 0.000000001
#endif

#include <limits.h>
#if INT_MAX < 0x7FFFFFFFL
#error "Only platforms with 32 bits or more are supported."
#endif


enum { SW_ONSTART, SW_OTHER, SW_ALL, SW_NONE, SINGLE_KEY };


#endif


/*
 * This file is part of enigma-suite-0.76, which is distributed under the terms
 * of the General Public License (GPL), version 2. See doc/COPYING for details.
 *
 * Copyright (C) 2005 Stefan Krah
 * 
 */
