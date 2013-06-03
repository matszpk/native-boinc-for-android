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

// This file contains compile time configuration parameters
// for the seti@home reference algorithm implementation.

#ifndef _ANALYZE_
#define _ANALYZE_

#define ISODD(a) (a)%2 != 0 ? true : false
#define IROUND(a) ( (int)  ((a) + 0.5f) )
#define LROUND(a) ( (long) ((a) + 0.5f) )
#define SQUARE(a) ((a) * (a))
typedef unsigned char BOOLEAN;

#define SPIKE_SCORE_HIGH 40          // a high number (extremely unlikely in noise)
#define AUTOCORR_SCORE_HIGH 40          // a high number (extremely unlikely in noise)

#define USE_PULSE
#define USE_TRIPLET

extern int t_offset_start;
extern int t_offset_stop;
extern float SlewRate;

extern float gauss_sigma;
extern float gauss_sigma_sq;

#define NEG_LN_ONE_HALF     0.693
#define EXP(a,b,c)          exp(-(NEG_LN_ONE_HALF * SQUARE((float)(a) - (float)(b))) / (float)(c))

#endif

// ==========================================================================

// General Notes

// Concerning the gaussian function macro EXP:
// The EXP macro defined here is used to generate the gaussian function
// f(x) = exp(-(C * x^2 / sigma^2)) in both fakedata and gaussfit.  Sigma
// is in arbitrary units and simply gives the width of the gaussian.  We
// give it a particular meaning by properly setting the value of  the
// constant C.  The meaning we give sigma here is that 2sigma is the
// half power beamwidth of the telescope.  We solve for C when x = sigma
// and f(x) = 1/2.  Thus we have f(x) = exp(-C).  Taking the natural log
// of both sides gives us ln(1/2) = -C.  And so C = -ln(1/2) = 0.693.
