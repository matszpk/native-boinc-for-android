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

// $Id: x86_float4.cpp,v 1.1.2.5 2007/07/16 15:36:56 korpela Exp $
//

// This file is empty is __i386__ is not defined
#include "sah_config.h"
#include "math.h"
#include "limits.h"
#include "x86_ops.h"
#include "x86_float4.h"

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_AMD64)

// polynomial coefficients for sine / cosine approximation in the chirp routine
const_float4 SS4F(-0.0046075748);
const_float4 CC3F(-0.0204391631);
const_float4 SS3F(0.0796819754);
const_float4 CC2F(0.2536086171);
const_float4 SS2F(-0.645963615);
const_float4 CC1F(-1.2336977925);
const_float4 SS1F(1.5707963235);
const_float4 ONE(1.0);
const_float4 TWO(2.0);

const_float4 SS1(1.5707963268);
const_float4 SS2(-0.6466386396);
const_float4 SS3(0.0679105987);
const_float4 SS4(-0.0011573807);
const_float4 CC1(-1.2341299769);
const_float4 CC2(0.2465220241);
const_float4 CC3(-0.0123926179);

const_float4 ZERO(0.0);
const_float4 THREE(3.0);
const_float4 RECIP_TWO(0.5);
const_float4 RECIP_THREE(1.0 / 3.0);
const_float4 RECIP_FOUR(0.25);
const_float4 RECIP_FIVE(0.2);
const_float4 RECIP_PI(1.0/M_PI);
const_float4 RECIP_TWOPI(0.5/M_PI);
const_float4 RECIP_HALFPI(2.0/M_PI);
const_float4 PI(M_PI);
const_float4 TWOPI(2.0*M_PI);
const_float4 HALFPI(0.5*M_PI);
const_float4 R_NEG(-1,1,-1,1);

// 2^23 (used by round())
const_float4 TWO_TO_23(8388608.0);
const_float4 INDGEN[]={const_float4(1,2,3,4),const_float4(5,6,7,8)};

#endif
