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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math/fp2intfp.h"
#include "arm_math/arm_log.h"

#ifdef ANDROID
extern void ifplog10(IntFp* a);
extern void ifplog(IntFp* a);

#ifdef TEST_ARMFUNC
static void compare_doubles(const char* funcname, double x, double a1, double a2)
{
    int mantisa;
    double diff = fabs(a1-a2);
    double frac = frexp(fmin(fabs(a1),fabs(a2)),&mantisa);
    if (diff*ldexp(1.0,-mantisa+1)>=6.220446049250313e-16)
        fprintf(stderr,"%s error:%1.18e,%1.18e,%1.18e,%1.18e\n",
                funcname,x,a1,a2,diff);
}
#endif

double arm_log10(double a)
{
    IntFp aifp;
#ifndef TEST_ARMFUNC
    double out;
#else
    double out,tocompare;
#endif
    fp_to_intfp(a,&aifp);
    ifplog10(&aifp);
    intfp_to_fp(&aifp, &out);
#ifdef TEST_ARMFUNC
    tocompare=log10(a);
    compare_doubles("log10",a,out,tocompare);
#endif
    return out;
}

double arm_log(double a)
{
    IntFp aifp;
#ifndef TEST_ARMFUNC
    double out;
#else
    double out,tocompare;
#endif
    fp_to_intfp(a,&aifp);
    ifplog(&aifp);
    intfp_to_fp(&aifp, &out);
#ifdef TEST_ARMFUNC
    tocompare=log(a);
    compare_doubles("log",a,out,tocompare);
#endif
    return out;
}
#endif
