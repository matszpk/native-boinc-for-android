// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions and
// distribute a linked executable.  You must obey the GNU General Public 
// License in all respects for all of the code used other than the FFT library
// itself.  Any modification required to support these libraries must be
// distributed in source code form.  If you modify this file, you may extend 
// this exception to your version of the file, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.

// $Id: pulsefind.h,v 1.3.2.3 2007/06/08 03:09:45 korpela Exp $

// constants for integerized period in preplanning
#define C3X2TO14 0xC000
#define C3X2TO13 0x6000

// constant for preplan limit
#define PPLANMAX 511

typedef float (*sum_func)( float *[], struct PoTPlan *);

struct PoTPlan {
    int            di;      // count                  0 = done
    float          *dest;   // (destination)
    int            tmp0;    // Tmp0 [+offset for 2sum]
    union     {int tmp1; int offset; };    // Tmp1 for 3,4,5sum - source offset for 2sum
    int            tmp2;    // Tmp2
    int            tmp3;    // Tmp3
    sum_func       fun_ptr; // pointer to sum routine
    unsigned long  cperiod; // *0xC000
    float          thresh;  // invert_lcgf()
    int            na;      // num_adds or num_adds_2
};


//      std::max is SLOW
#define UNSTDMAX(a,b)  (((a) > (b)) ? (a) : (b))

#define FOLDTBLEN 32

struct FoldSet {
    sum_func     *f3;       // to table for fold by 3
    sum_func     *f4;       // to table for fold by 4
    sum_func     *f5;       // to table for fold by 5
    sum_func     *f2;       // to table for fold by 2
    sum_func     *f2AL;     // to table for fold by 2 needing tmp0 aligned
    const char   *name;     // to string literal name of set
};


// forward declare folding sets

extern FoldSet AVXfold_a;       // in analyzeFuncs_avx.cpp
extern FoldSet AVXfold_c;       // in analyzeFuncs_avx.cpp
extern FoldSet sse_ben_fold;    // in analyzeFuncs_sse.cpp
extern FoldSet BHSSEfold;       // in analyzeFuncs_sse.cpp
extern FoldSet AKSSEfold;       // in analyzeFuncs_sse.cpp
extern FoldSet AKavfold;        // in analyzeFuncs_altivec.cpp
extern FoldSet swifold;         // in Pulsefind - default set
extern FoldSet Foldmain;        // in Pulsefind - used set

// routines in pulsefind.cpp

int find_triplets(const float * fp_PulsePot, int PulsePotLen, float triplet_thresh, int TOffset, int ul_PoT);

int find_pulse(const float * fp_PulsePot, int PulsePotLen, float pulse_thresh, int TOffset, int ul_PoT);

int CopyFoldSet(FoldSet *dst, FoldSet *src);
