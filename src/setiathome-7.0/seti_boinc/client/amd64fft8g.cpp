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

// AMD optimizations by Evandro Menezes

#ifdef USE_AMD_OPT_CODE

#include <climits>
#include <xmmintrin.h>

static const int as [4] [4] = {{0, 0, INT_MIN, INT_MIN},  // {-, -, +, +}
                               {0, 0, 0,       INT_MIN},  // {-, +, +, +}
                               {0, INT_MIN, 0, INT_MIN},  // {-, +, -, +}
                               {INT_MIN, 0, INT_MIN, 0}}; // {+, -, +, -}
static const __m128 mmpp = *(__m128 *) (as [0] + 0),
                    mppp = *(__m128 *) (as [1] + 0),
                    mpmp = *(__m128 *) (as [2] + 0),
                    pmpm = *(__m128 *) (as [3] + 0);

void cftfsub(int n, float *a, float *w)
{
    void cft1st(int n, float *a, float *w);
    void cftmdl(int n, int l, float *a, float *w);
    int j, j1, j2, j3, l;
    __m128 x10, x32, y10, y32, zz;

    zz = _mm_setzero_ps ();

    l = 2;
    if (n >= 16) {
        cft1st(n, a, w);
        l = 16;
        while ((l << 3) <= n) {
            cftmdl(n, l, a, w);
            l <<= 3;
        }
    }
    if ((l << 1) < n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            // x0r = a[j] + a[j1];
            // x0i = a[j + 1] + a[j1 + 1];
            x10 = _mm_loadl_pi (zz, (__m64 *) (a + j));
            y10 = _mm_loadl_pi (zz, (__m64 *) (a + j1));
            // x1r = a[j] - a[j1];
            // x1i = a[j + 1] - a[j1 + 1];
            x10 = _mm_movelh_ps (x10, x10);
            y10 = _mm_movelh_ps (y10, y10);
            y10 = _mm_xor_ps (mmpp, y10);
            x10 = _mm_add_ps (x10, y10);
            // x2r = a[j2] + a[j3];
            // x2i = a[j2 + 1] + a[j3 + 1];
            x32 = _mm_loadl_pi (zz, (__m64 *) (a + j2));
            y32 = _mm_loadl_pi (zz, (__m64 *) (a + j3));
            // x3r = a[j2] - a[j3];
            // x3i = a[j2 + 1] - a[j3 + 1];
            x32 = _mm_movelh_ps (x32, x32);
            y32 = _mm_movelh_ps (y32, y32);
            y32 = _mm_xor_ps (mmpp, y32);
            x32 = _mm_add_ps (x32, y32);

            // a[j] = x0r + x2r;
            // a[j + 1] = x0i + x2i;
            // a[j1] = x1r - x3i;
            // a[j1 + 1] = x1i + x3r;
            x32 = _mm_xor_ps (mppp, x32);
            x32 = _mm_shuffle_ps (x32, x32, _MM_SHUFFLE (2, 3, 1, 0));
            y10 = _mm_add_ps (x10, x32);
            _mm_storel_pi ((__m64 *) (a + j),  y10);
            _mm_storeh_pi ((__m64 *) (a + j1), y10);
            // a[j2] = x0r - x2r;
            // a[j2 + 1] = x0i - x2i;
            // a[j3] = x1r + x3i;
            // a[j3 + 1] = x1i - x3r;
            y32 = _mm_sub_ps (x10, x32);
            _mm_storel_pi ((__m64 *) (a + j2), y32);
            _mm_storeh_pi ((__m64 *) (a + j3), y32);
        }
    } else if ((l << 1) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            // x0r = a[j] - a[j1];
            // x0i = a[j + 1] - a[j1 + 1];
            x10 = _mm_loadl_pi (zz, (__m64 *) (a + j));
            x32 = _mm_loadl_pi (zz, (__m64 *) (a + j1));
            y10 = _mm_sub_ps (x10, x32);
            // a[j] += a[j1];
            // a[j + 1] += a[j1 + 1];
            y32 = _mm_add_ps (x10, x32);
            _mm_storel_pi ((__m64 *) (a + j), y32);
            // a[j1] = x0r;
            // a[j1 + 1] = x0i;
            _mm_storel_pi ((__m64 *) (a + j1), y10);
        }
    }
}
#endif   // USE_AMD_OPT_CODE
