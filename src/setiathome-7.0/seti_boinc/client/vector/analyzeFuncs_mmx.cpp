/******************
 *
 * opt_mmx.cpp
 * 
 * Description: Intel & AMD MMX optimized functions
 *  CPUs: Intel Pentium II and beyond,  AMD K6 -  and beyond
 *
 */

// Copyright 2006 Regents of the University of California

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

#include "x86_ops.h"

typedef __m64 MMX;

#define s_begin()           _mm_empty()
#define s_end()             _mm_empty()

#define s_get64( ptr )		*( (MMX *) (ptr) )
#define s_put64( ptr, aa )	( *((MMX *) (ptr)) = aa )
#define s_put64nc			_mm_stream_pi


//
//
//
void __fastcall copy_MMX(char *dest, const char *src, int blocks)
	{
	MMX m0,m1,m2,m3,m4,m5,m6,m7;

	MMX *m_src = (MMX *)src;
	MMX *m_dst = (MMX *)dest;
	s_begin();
	while( blocks-- )
		{
		m0 = s_get64( m_src + 0 );
		m1 = s_get64( m_src + 1 );
		m2 = s_get64( m_src + 2 );
		m3 = s_get64( m_src + 3 );
		m4 = s_get64( m_src + 4 );
		m5 = s_get64( m_src + 5 );
		m6 = s_get64( m_src + 6 );
		m7 = s_get64( m_src + 7 );
		m_src += 8;
		s_put64( m_dst + 0, m0 );
		s_put64( m_dst + 1, m1 );
		s_put64( m_dst + 2, m2 );
		s_put64( m_dst + 3, m3 );
		s_put64( m_dst + 4, m4 );
		s_put64( m_dst + 5, m5 );
		s_put64( m_dst + 6, m6 );
		s_put64( m_dst + 7, m7 );
		m_dst += 8;
		}
	s_end();
	}

//
// copyMMXnt - Use Non temporal writes, 
//   only available with 3DNow+ and SSE+ - Athlon and Pentium III
//
void __fastcall copy_MMXnt(char *dest, const char *src, int blocks)
	{
	MMX m0,m1,m2,m3,m4,m5,m6,m7;

	MMX *m_src = (MMX *)src;
	MMX *m_dst = (MMX *)dest;
	s_begin();
	while( blocks-- )
		{
		s_fetch( m_src + 16 );
		m0 = s_get64( m_src + 0 );
		m1 = s_get64( m_src + 1 );
		m2 = s_get64( m_src + 2 );
		m3 = s_get64( m_src + 3 );
		m4 = s_get64( m_src + 4 );
		m5 = s_get64( m_src + 5 );
		m6 = s_get64( m_src + 6 );
		m7 = s_get64( m_src + 7 );
		m_src += 8;
		s_put64nc( m_dst + 0, m0 );
		s_put64nc( m_dst + 1, m1 );
		s_put64nc( m_dst + 2, m2 );
		s_put64nc( m_dst + 3, m3 );
		s_put64nc( m_dst + 4, m4 );
		s_put64nc( m_dst + 5, m5 );
		s_put64nc( m_dst + 6, m6 );
		s_put64nc( m_dst + 7, m7 );
		m_dst += 8;
		}
	s_end();
	s_fence_writes();
	}
