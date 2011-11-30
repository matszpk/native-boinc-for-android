/* Copyright 2011 Mateusz Szpakowski

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __FP2INTFP_H__
#define __FP2INTFP_H__

#include <stdint.h>

#include <stdint.h>

typedef uint32_t IntFp[3];

extern void fp_to_intfp(double in, IntFp* out);
extern void intfp_to_fp(IntFp* in, double* out);

#endif