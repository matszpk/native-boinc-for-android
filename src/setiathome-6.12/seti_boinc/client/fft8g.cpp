// This code is from the General Purpose FFT (Fast Fourier/Cosine/Sine Transform) Package.
// Copyright Takuya OOURA, 1996-2001 
// (Email: ooura@kurims.kyoto-u.ac.jp or ooura@mmm.t.u-tokyo.ac.jp) 
//
// You may use, copy, modify and distribute this code for any purpose 
// (include commercial use) and without fee. Please refer to this package 
// when you modify this code. 

// $Id: fft8g.cpp,v 1.4.2.3 2006/12/14 22:21:45 korpela Exp $
/*
NOTE ON USE BY SETI@HOME :  All floating point objects were changed
    from double precision to single precision.
    The function prototypes below were placed
    in file fft8g.h.  
    Also, the following unused transforms were
    removed from the source:
     rdft: Real Discrete Fourier Transform
         ddct: Discrete Cosine Transform
         ddst: Discrete Sine Transform
         dfct: Cosine Transform of RDFT (Real Symmetric DFT)
         dfst: Sine Transform of RDFT (Real Anti-symmetric DFT)
    
    Modified to use sincosf/sinf/cosf/atanf.  Implementations of these are
    in sincos.h for machines without
 
 
Fast Fourier/Cosine/Sine Transform
    dimension   :one
    data length :power of 2
    decimation  :frequency
    radix       :8, 4, 2
    data        :inplace
    table       :use
functions
    cdft: Complex Discrete Fourier Transform
function prototypes
    void cdft(int, int, float *, int *, float *);
    void rdft(int, int, float *, int *, float *);
    void ddct(int, int, float *, int *, float *);
    void ddst(int, int, float *, int *, float *);
    void dfct(int, float *, float *, int *, float *);
    void dfst(int, float *, float *, int *, float *);
 
 
-------- Complex DFT (Discrete Fourier Transform) --------
    [definition]
        <case1>
            X[k] = sum_j=0^n-1 x[j]*exp(2*pi*i*j*k/n), 0<=k<n
        <case2>
            X[k] = sum_j=0^n-1 x[j]*exp(-2*pi*i*j*k/n), 0<=k<n
        (notes: sum_j=0^n-1 is a summation from j=0 to n-1)
    [usage]
        <case1>
            ip[0] = 0; // first time only
            cdft(2*n, 1, a, ip, w);
        <case2>
            ip[0] = 0; // first time only
            cdft(2*n, -1, a, ip, w);
    [parameters]
        2*n            :data length (int)
                        n >= 1, n = power of 2
        a[0...2*n-1]   :input/output data (float *)
                        input data
                            a[2*j] = Re(x[j]), 
                            a[2*j+1] = Im(x[j]), 0<=j<n
                        output data
                            a[2*k] = Re(X[k]), 
                            a[2*k+1] = Im(X[k]), 0<=k<n
        ip[0...*]      :work area for bit reversal (int *)
                        length of ip >= 2+sqrt(n)
                        strictly, 
                        length of ip >= 
                            2+(1<<(int)(log(n+0.5)/log(2))/2).
                        ip[0],ip[1] are pointers of the cos/sin table.
        w[0...n/2-1]   :cos/sin table (float *)
                        w[],ip[] are initialized if ip[0] == 0.
    [remark]
        Inverse of 
            cdft(2*n, -1, a, ip, w);
        is 
            cdft(2*n, 1, a, ip, w);
            for (j = 0; j <= 2 * n - 1; j++) {
                a[j] *= 1.0 / n;
            }
        .
 
 
Appendix :
    The cos/sin table is recalculated when the larger table required.
    w[] and ip[] are compatible with all routines.
*/

#include "sah_config.h"
#include "sincos.h"
#include "s_util.h"

void fft8g_start() {}


void cdft(int n, int isgn, sah_complex *aa, int *ip, float *w) {
  void makewt(int nw, int *ip, float *w);
  void bitrv2(int n, int *ip, float *a);
  void bitrv2conj(int n, int *ip, float *a);
  void cftfsub(int n, float *a, float *w);
  void cftbsub(int n, float *a, float *w);

  float *a=(float *)aa;

  if (n > (ip[0] << 2)) {
    makewt(n >> 2, ip, w);
  }
  if (n > 4) {
    if (isgn >= 0) {
      bitrv2(n, ip + 2, a);
      cftfsub(n, a, w);
    } else {
      bitrv2conj(n, ip + 2, a);
      cftbsub(n, a, w);
    }
  } else if (n == 4) {
    cftfsub(n, a, w);
  }
}


/* -------- initializing routines -------- */


#include <cmath>

void makewt(int nw, int *ip, float *w) {
  void bitrv2(int n, int *ip, float *a);
  int j, nwh;
  float delta, x, y;

  ip[0] = nw;
  ip[1] = 1;
  if (nw > 2) {
    nwh = nw >> 1;
    delta = atanf(1.0) / nwh;
    w[0] = 1;
    w[1] = 0;
    w[nwh] = cosf(delta * nwh);
    w[nwh + 1] = w[nwh];
    if (nwh > 2) {
      for (j = 2; j < nwh; j += 2) {
        sincosf(delta*j,&y,&x);
        w[j] = x;
        w[j + 1] = y;
        w[nw - j] = y;
        w[nw - j + 1] = x;
      }
      for (j = nwh - 2; j >= 2; j -= 2) {
        x = w[2 * j];
        y = w[2 * j + 1];
        w[nwh + j] = x;
        w[nwh + j + 1] = y;
      }
      bitrv2(nw, ip + 2, w);
    }
  }
}


void makect(int nc, int *ip, float *c) {
  int j, nch;
  float delta;

  ip[1] = nc;
  if (nc > 1) {
    nch = nc >> 1;
    delta = atanf(1.0f) / nch;
    c[0] = cosf(delta * nch);
    c[nch] = (0.5f * c[0]);
    for (j = 1; j < nch; j++) {
      sincosf(delta*j,c+nc-j,c+j);
      c[j]*=0.5f;
      c[nc-j]*=0.5f;
    }
  }
}


/* -------- child routines -------- */


void bitrv2(int n, int *ip, float *a) {
  int j, j1, k, k1, l, m, m2;
  float xr, xi, yr, yi;

  ip[0] = 0;
  l = n;
  m = 1;
  while ((m << 3) < l) {
    l >>= 1;
    for (j = 0; j < m; j++) {
      ip[m + j] = ip[j] + l;
    }
    m <<= 1;
  }
  m2 = 2 * m;
  if ((m << 3) == l) {
    for (k = 0; k < m; k++) {
      for (j = 0; j < k; j++) {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 -= m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
      j1 = 2 * k + m2 + ip[k];
      k1 = j1 + m2;
      xr = a[j1];
      xi = a[j1 + 1];
      yr = a[k1];
      yi = a[k1 + 1];
      a[j1] = yr;
      a[j1 + 1] = yi;
      a[k1] = xr;
      a[k1 + 1] = xi;
    }
  } else {
    for (k = 1; k < m; k++) {
      for (j = 0; j < k; j++) {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += m2;
        xr = a[j1];
        xi = a[j1 + 1];
        yr = a[k1];
        yi = a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
    }
  }
}


void bitrv2conj(int n, int *ip, float *a) {
  int j, j1, k, k1, l, m, m2;
  float xr, xi, yr, yi;

  ip[0] = 0;
  l = n;
  m = 1;
  while ((m << 3) < l) {
    l >>= 1;
    for (j = 0; j < m; j++) {
      ip[m + j] = ip[j] + l;
    }
    m <<= 1;
  }
  m2 = 2 * m;
  if ((m << 3) == l) {
    for (k = 0; k < m; k++) {
      for (j = 0; j < k; j++) {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 -= m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += 2 * m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
      k1 = 2 * k + ip[k];
      a[k1 + 1] = -a[k1 + 1];
      j1 = k1 + m2;
      k1 = j1 + m2;
      xr = a[j1];
      xi = -a[j1 + 1];
      yr = a[k1];
      yi = -a[k1 + 1];
      a[j1] = yr;
      a[j1 + 1] = yi;
      a[k1] = xr;
      a[k1 + 1] = xi;
      k1 += m2;
      a[k1 + 1] = -a[k1 + 1];
    }
  } else {
    a[1] = -a[1];
    a[m2 + 1] = -a[m2 + 1];
    for (k = 1; k < m; k++) {
      for (j = 0; j < k; j++) {
        j1 = 2 * j + ip[k];
        k1 = 2 * k + ip[j];
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
        j1 += m2;
        k1 += m2;
        xr = a[j1];
        xi = -a[j1 + 1];
        yr = a[k1];
        yi = -a[k1 + 1];
        a[j1] = yr;
        a[j1 + 1] = yi;
        a[k1] = xr;
        a[k1 + 1] = xi;
      }
      k1 = 2 * k + ip[k];
      a[k1 + 1] = -a[k1 + 1];
      a[k1 + m2 + 1] = -a[k1 + m2 + 1];
    }
  }
}


void cftfsub(int n, float *a, float *w) {
  void cft1st(int n, float *a, float *w);
  void cftmdl(int n, int l, float *a, float *w);
  int j, j1, j2, j3, l;
  float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

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
      x0r = a[j] + a[j1];
      x0i = a[j + 1] + a[j1 + 1];
      x1r = a[j] - a[j1];
      x1i = a[j + 1] - a[j1 + 1];
      x2r = a[j2] + a[j3];
      x2i = a[j2 + 1] + a[j3 + 1];
      x3r = a[j2] - a[j3];
      x3i = a[j2 + 1] - a[j3 + 1];
      a[j] = x0r + x2r;
      a[j + 1] = x0i + x2i;
      a[j2] = x0r - x2r;
      a[j2 + 1] = x0i - x2i;
      a[j1] = x1r - x3i;
      a[j1 + 1] = x1i + x3r;
      a[j3] = x1r + x3i;
      a[j3 + 1] = x1i - x3r;
    }
  } else if ((l << 1) == n) {
    for (j = 0; j < l; j += 2) {
      j1 = j + l;
      x0r = a[j] - a[j1];
      x0i = a[j + 1] - a[j1 + 1];
      a[j] += a[j1];
      a[j + 1] += a[j1 + 1];
      a[j1] = x0r;
      a[j1 + 1] = x0i;
    }
  }
}


void cftbsub(int n, float *a, float *w) {
  void cft1st(int n, float *a, float *w);
  void cftmdl(int n, int l, float *a, float *w);
  int j, j1, j2, j3, j4, j5, j6, j7, l;
  float wn4r, x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

  l = 2;
  if (n > 16) {
    cft1st(n, a, w);
    l = 16;
    while ((l << 3) < n) {
      cftmdl(n, l, a, w);
      l <<= 3;
    }
  }
  if ((l << 2) < n) {
    wn4r = w[2];
    for (j = 0; j < l; j += 2) {
      j1 = j + l;
      j2 = j1 + l;
      j3 = j2 + l;
      j4 = j3 + l;
      j5 = j4 + l;
      j6 = j5 + l;
      j7 = j6 + l;
      x0r = a[j] + a[j1];
      x0i = -a[j + 1] - a[j1 + 1];
      x1r = a[j] - a[j1];
      x1i = -a[j + 1] + a[j1 + 1];
      x2r = a[j2] + a[j3];
      x2i = a[j2 + 1] + a[j3 + 1];
      x3r = a[j2] - a[j3];
      x3i = a[j2 + 1] - a[j3 + 1];
      y0r = x0r + x2r;
      y0i = x0i - x2i;
      y2r = x0r - x2r;
      y2i = x0i + x2i;
      y1r = x1r - x3i;
      y1i = x1i - x3r;
      y3r = x1r + x3i;
      y3i = x1i + x3r;
      x0r = a[j4] + a[j5];
      x0i = a[j4 + 1] + a[j5 + 1];
      x1r = a[j4] - a[j5];
      x1i = a[j4 + 1] - a[j5 + 1];
      x2r = a[j6] + a[j7];
      x2i = a[j6 + 1] + a[j7 + 1];
      x3r = a[j6] - a[j7];
      x3i = a[j6 + 1] - a[j7 + 1];
      y4r = x0r + x2r;
      y4i = x0i + x2i;
      y6r = x0r - x2r;
      y6i = x0i - x2i;
      x0r = x1r - x3i;
      x0i = x1i + x3r;
      x2r = x1r + x3i;
      x2i = x1i - x3r;
      y5r = wn4r * (x0r - x0i);
      y5i = wn4r * (x0r + x0i);
      y7r = wn4r * (x2r - x2i);
      y7i = wn4r * (x2r + x2i);
      a[j1] = y1r + y5r;
      a[j1 + 1] = y1i - y5i;
      a[j5] = y1r - y5r;
      a[j5 + 1] = y1i + y5i;
      a[j3] = y3r - y7i;
      a[j3 + 1] = y3i - y7r;
      a[j7] = y3r + y7i;
      a[j7 + 1] = y3i + y7r;
      a[j] = y0r + y4r;
      a[j + 1] = y0i - y4i;
      a[j4] = y0r - y4r;
      a[j4 + 1] = y0i + y4i;
      a[j2] = y2r - y6i;
      a[j2 + 1] = y2i - y6r;
      a[j6] = y2r + y6i;
      a[j6 + 1] = y2i + y6r;
    }
  } else if ((l << 2) == n) {
    for (j = 0; j < l; j += 2) {
      j1 = j + l;
      j2 = j1 + l;
      j3 = j2 + l;
      x0r = a[j] + a[j1];
      x0i = -a[j + 1] - a[j1 + 1];
      x1r = a[j] - a[j1];
      x1i = -a[j + 1] + a[j1 + 1];
      x2r = a[j2] + a[j3];
      x2i = a[j2 + 1] + a[j3 + 1];
      x3r = a[j2] - a[j3];
      x3i = a[j2 + 1] - a[j3 + 1];
      a[j] = x0r + x2r;
      a[j + 1] = x0i - x2i;
      a[j2] = x0r - x2r;
      a[j2 + 1] = x0i + x2i;
      a[j1] = x1r - x3i;
      a[j1 + 1] = x1i - x3r;
      a[j3] = x1r + x3i;
      a[j3 + 1] = x1i + x3r;
    }
  } else {
    for (j = 0; j < l; j += 2) {
      j1 = j + l;
      x0r = a[j] - a[j1];
      x0i = -a[j + 1] + a[j1 + 1];
      a[j] += a[j1];
      a[j + 1] = -a[j + 1] - a[j1 + 1];
      a[j1] = x0r;
      a[j1 + 1] = x0i;
    }
  }
}


void cft1st(int n, float *a, float *w) {
  int j, k1;
  float wn4r, wtmp, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i,
  wk4r, wk4i, wk5r, wk5i, wk6r, wk6i, wk7r, wk7i;
  float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

  wn4r = w[2];
  x0r = a[0] + a[2];
  x0i = a[1] + a[3];
  x1r = a[0] - a[2];
  x1i = a[1] - a[3];
  x2r = a[4] + a[6];
  x2i = a[5] + a[7];
  x3r = a[4] - a[6];
  x3i = a[5] - a[7];
  y0r = x0r + x2r;
  y0i = x0i + x2i;
  y2r = x0r - x2r;
  y2i = x0i - x2i;
  y1r = x1r - x3i;
  y1i = x1i + x3r;
  y3r = x1r + x3i;
  y3i = x1i - x3r;
  x0r = a[8] + a[10];
  x0i = a[9] + a[11];
  x1r = a[8] - a[10];
  x1i = a[9] - a[11];
  x2r = a[12] + a[14];
  x2i = a[13] + a[15];
  x3r = a[12] - a[14];
  x3i = a[13] - a[15];
  y4r = x0r + x2r;
  y4i = x0i + x2i;
  y6r = x0r - x2r;
  y6i = x0i - x2i;
  x0r = x1r - x3i;
  x0i = x1i + x3r;
  x2r = x1r + x3i;
  x2i = x1i - x3r;
  y5r = wn4r * (x0r - x0i);
  y5i = wn4r * (x0r + x0i);
  y7r = wn4r * (x2r - x2i);
  y7i = wn4r * (x2r + x2i);
  a[2] = y1r + y5r;
  a[3] = y1i + y5i;
  a[10] = y1r - y5r;
  a[11] = y1i - y5i;
  a[6] = y3r - y7i;
  a[7] = y3i + y7r;
  a[14] = y3r + y7i;
  a[15] = y3i - y7r;
  a[0] = y0r + y4r;
  a[1] = y0i + y4i;
  a[8] = y0r - y4r;
  a[9] = y0i - y4i;
  a[4] = y2r - y6i;
  a[5] = y2i + y6r;
  a[12] = y2r + y6i;
  a[13] = y2i - y6r;
  if (n > 16) {
    wk1r = w[4];
    wk1i = w[5];
    x0r = a[16] + a[18];
    x0i = a[17] + a[19];
    x1r = a[16] - a[18];
    x1i = a[17] - a[19];
    x2r = a[20] + a[22];
    x2i = a[21] + a[23];
    x3r = a[20] - a[22];
    x3i = a[21] - a[23];
    y0r = x0r + x2r;
    y0i = x0i + x2i;
    y2r = x0r - x2r;
    y2i = x0i - x2i;
    y1r = x1r - x3i;
    y1i = x1i + x3r;
    y3r = x1r + x3i;
    y3i = x1i - x3r;
    x0r = a[24] + a[26];
    x0i = a[25] + a[27];
    x1r = a[24] - a[26];
    x1i = a[25] - a[27];
    x2r = a[28] + a[30];
    x2i = a[29] + a[31];
    x3r = a[28] - a[30];
    x3i = a[29] - a[31];
    y4r = x0r + x2r;
    y4i = x0i + x2i;
    y6r = x0r - x2r;
    y6i = x0i - x2i;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    x2r = x1r + x3i;
    x2i = x3r - x1i;
    y5r = wk1i * x0r - wk1r * x0i;
    y5i = wk1i * x0i + wk1r * x0r;
    y7r = wk1r * x2r + wk1i * x2i;
    y7i = wk1r * x2i - wk1i * x2r;
    x0r = wk1r * y1r - wk1i * y1i;
    x0i = wk1r * y1i + wk1i * y1r;
    a[18] = x0r + y5r;
    a[19] = x0i + y5i;
    a[26] = y5i - x0i;
    a[27] = x0r - y5r;
    x0r = wk1i * y3r - wk1r * y3i;
    x0i = wk1i * y3i + wk1r * y3r;
    a[22] = x0r - y7r;
    a[23] = x0i + y7i;
    a[30] = y7i - x0i;
    a[31] = x0r + y7r;
    a[16] = y0r + y4r;
    a[17] = y0i + y4i;
    a[24] = y4i - y0i;
    a[25] = y0r - y4r;
    x0r = y2r - y6i;
    x0i = y2i + y6r;
    a[20] = wn4r * (x0r - x0i);
    a[21] = wn4r * (x0i + x0r);
    x0r = y6r - y2i;
    x0i = y2r + y6i;
    a[28] = wn4r * (x0r - x0i);
    a[29] = wn4r * (x0i + x0r);
    k1 = 4;
    for (j = 32; j < n; j += 16) {
      k1 += 4;
      wk1r = w[k1];
      wk1i = w[k1 + 1];
      wk2r = w[k1 + 2];
      wk2i = w[k1 + 3];
      wtmp = 2 * wk2i;
      wk3r = wk1r - wtmp * wk1i;
      wk3i = wtmp * wk1r - wk1i;
      wk4r = 1 - wtmp * wk2i;
      wk4i = wtmp * wk2r;
      wtmp = 2 * wk4i;
      wk5r = wk3r - wtmp * wk1i;
      wk5i = wtmp * wk1r - wk3i;
      wk6r = wk2r - wtmp * wk2i;
      wk6i = wtmp * wk2r - wk2i;
      wk7r = wk1r - wtmp * wk3i;
      wk7i = wtmp * wk3r - wk1i;
      x0r = a[j] + a[j + 2];
      x0i = a[j + 1] + a[j + 3];
      x1r = a[j] - a[j + 2];
      x1i = a[j + 1] - a[j + 3];
      x2r = a[j + 4] + a[j + 6];
      x2i = a[j + 5] + a[j + 7];
      x3r = a[j + 4] - a[j + 6];
      x3i = a[j + 5] - a[j + 7];
      y0r = x0r + x2r;
      y0i = x0i + x2i;
      y2r = x0r - x2r;
      y2i = x0i - x2i;
      y1r = x1r - x3i;
      y1i = x1i + x3r;
      y3r = x1r + x3i;
      y3i = x1i - x3r;
      x0r = a[j + 8] + a[j + 10];
      x0i = a[j + 9] + a[j + 11];
      x1r = a[j + 8] - a[j + 10];
      x1i = a[j + 9] - a[j + 11];
      x2r = a[j + 12] + a[j + 14];
      x2i = a[j + 13] + a[j + 15];
      x3r = a[j + 12] - a[j + 14];
      x3i = a[j + 13] - a[j + 15];
      y4r = x0r + x2r;
      y4i = x0i + x2i;
      y6r = x0r - x2r;
      y6i = x0i - x2i;
      x0r = x1r - x3i;
      x0i = x1i + x3r;
      x2r = x1r + x3i;
      x2i = x1i - x3r;
      y5r = wn4r * (x0r - x0i);
      y5i = wn4r * (x0r + x0i);
      y7r = wn4r * (x2r - x2i);
      y7i = wn4r * (x2r + x2i);
      x0r = y1r + y5r;
      x0i = y1i + y5i;
      a[j + 2] = wk1r * x0r - wk1i * x0i;
      a[j + 3] = wk1r * x0i + wk1i * x0r;
      x0r = y1r - y5r;
      x0i = y1i - y5i;
      a[j + 10] = wk5r * x0r - wk5i * x0i;
      a[j + 11] = wk5r * x0i + wk5i * x0r;
      x0r = y3r - y7i;
      x0i = y3i + y7r;
      a[j + 6] = wk3r * x0r - wk3i * x0i;
      a[j + 7] = wk3r * x0i + wk3i * x0r;
      x0r = y3r + y7i;
      x0i = y3i - y7r;
      a[j + 14] = wk7r * x0r - wk7i * x0i;
      a[j + 15] = wk7r * x0i + wk7i * x0r;
      a[j] = y0r + y4r;
      a[j + 1] = y0i + y4i;
      x0r = y0r - y4r;
      x0i = y0i - y4i;
      a[j + 8] = wk4r * x0r - wk4i * x0i;
      a[j + 9] = wk4r * x0i + wk4i * x0r;
      x0r = y2r - y6i;
      x0i = y2i + y6r;
      a[j + 4] = wk2r * x0r - wk2i * x0i;
      a[j + 5] = wk2r * x0i + wk2i * x0r;
      x0r = y2r + y6i;
      x0i = y2i - y6r;
      a[j + 12] = wk6r * x0r - wk6i * x0i;
      a[j + 13] = wk6r * x0i + wk6i * x0r;
    }
  }
}


void cftmdl(int n, int l, float *a, float *w) {
  int j, j1, j2, j3, j4, j5, j6, j7, k, k1, m;
  float wn4r, wtmp, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i,
  wk4r, wk4i, wk5r, wk5i, wk6r, wk6i, wk7r, wk7i;
  float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
  y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
  y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

  m = l << 3;
  wn4r = w[2];
  for (j = 0; j < l; j += 2) {
    j1 = j + l;
    j2 = j1 + l;
    j3 = j2 + l;
    j4 = j3 + l;
    j5 = j4 + l;
    j6 = j5 + l;
    j7 = j6 + l;
    x0r = a[j] + a[j1];
    x0i = a[j + 1] + a[j1 + 1];
    x1r = a[j] - a[j1];
    x1i = a[j + 1] - a[j1 + 1];
    x2r = a[j2] + a[j3];
    x2i = a[j2 + 1] + a[j3 + 1];
    x3r = a[j2] - a[j3];
    x3i = a[j2 + 1] - a[j3 + 1];
    y0r = x0r + x2r;
    y0i = x0i + x2i;
    y2r = x0r - x2r;
    y2i = x0i - x2i;
    y1r = x1r - x3i;
    y1i = x1i + x3r;
    y3r = x1r + x3i;
    y3i = x1i - x3r;
    x0r = a[j4] + a[j5];
    x0i = a[j4 + 1] + a[j5 + 1];
    x1r = a[j4] - a[j5];
    x1i = a[j4 + 1] - a[j5 + 1];
    x2r = a[j6] + a[j7];
    x2i = a[j6 + 1] + a[j7 + 1];
    x3r = a[j6] - a[j7];
    x3i = a[j6 + 1] - a[j7 + 1];
    y4r = x0r + x2r;
    y4i = x0i + x2i;
    y6r = x0r - x2r;
    y6i = x0i - x2i;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    x2r = x1r + x3i;
    x2i = x1i - x3r;
    y5r = wn4r * (x0r - x0i);
    y5i = wn4r * (x0r + x0i);
    y7r = wn4r * (x2r - x2i);
    y7i = wn4r * (x2r + x2i);
    a[j1] = y1r + y5r;
    a[j1 + 1] = y1i + y5i;
    a[j5] = y1r - y5r;
    a[j5 + 1] = y1i - y5i;
    a[j3] = y3r - y7i;
    a[j3 + 1] = y3i + y7r;
    a[j7] = y3r + y7i;
    a[j7 + 1] = y3i - y7r;
    a[j] = y0r + y4r;
    a[j + 1] = y0i + y4i;
    a[j4] = y0r - y4r;
    a[j4 + 1] = y0i - y4i;
    a[j2] = y2r - y6i;
    a[j2 + 1] = y2i + y6r;
    a[j6] = y2r + y6i;
    a[j6 + 1] = y2i - y6r;
  }
  if (m < n) {
    wk1r = w[4];
    wk1i = w[5];
    for (j = m; j < l + m; j += 2) {
      j1 = j + l;
      j2 = j1 + l;
      j3 = j2 + l;
      j4 = j3 + l;
      j5 = j4 + l;
      j6 = j5 + l;
      j7 = j6 + l;
      x0r = a[j] + a[j1];
      x0i = a[j + 1] + a[j1 + 1];
      x1r = a[j] - a[j1];
      x1i = a[j + 1] - a[j1 + 1];
      x2r = a[j2] + a[j3];
      x2i = a[j2 + 1] + a[j3 + 1];
      x3r = a[j2] - a[j3];
      x3i = a[j2 + 1] - a[j3 + 1];
      y0r = x0r + x2r;
      y0i = x0i + x2i;
      y2r = x0r - x2r;
      y2i = x0i - x2i;
      y1r = x1r - x3i;
      y1i = x1i + x3r;
      y3r = x1r + x3i;
      y3i = x1i - x3r;
      x0r = a[j4] + a[j5];
      x0i = a[j4 + 1] + a[j5 + 1];
      x1r = a[j4] - a[j5];
      x1i = a[j4 + 1] - a[j5 + 1];
      x2r = a[j6] + a[j7];
      x2i = a[j6 + 1] + a[j7 + 1];
      x3r = a[j6] - a[j7];
      x3i = a[j6 + 1] - a[j7 + 1];
      y4r = x0r + x2r;
      y4i = x0i + x2i;
      y6r = x0r - x2r;
      y6i = x0i - x2i;
      x0r = x1r - x3i;
      x0i = x1i + x3r;
      x2r = x1r + x3i;
      x2i = x3r - x1i;
      y5r = wk1i * x0r - wk1r * x0i;
      y5i = wk1i * x0i + wk1r * x0r;
      y7r = wk1r * x2r + wk1i * x2i;
      y7i = wk1r * x2i - wk1i * x2r;
      x0r = wk1r * y1r - wk1i * y1i;
      x0i = wk1r * y1i + wk1i * y1r;
      a[j1] = x0r + y5r;
      a[j1 + 1] = x0i + y5i;
      a[j5] = y5i - x0i;
      a[j5 + 1] = x0r - y5r;
      x0r = wk1i * y3r - wk1r * y3i;
      x0i = wk1i * y3i + wk1r * y3r;
      a[j3] = x0r - y7r;
      a[j3 + 1] = x0i + y7i;
      a[j7] = y7i - x0i;
      a[j7 + 1] = x0r + y7r;
      a[j] = y0r + y4r;
      a[j + 1] = y0i + y4i;
      a[j4] = y4i - y0i;
      a[j4 + 1] = y0r - y4r;
      x0r = y2r - y6i;
      x0i = y2i + y6r;
      a[j2] = wn4r * (x0r - x0i);
      a[j2 + 1] = wn4r * (x0i + x0r);
      x0r = y6r - y2i;
      x0i = y2r + y6i;
      a[j6] = wn4r * (x0r - x0i);
      a[j6 + 1] = wn4r * (x0i + x0r);
    }
    k1 = 4;
    for (k = 2 * m; k < n; k += m) {
      k1 += 4;
      wk1r = w[k1];
      wk1i = w[k1 + 1];
      wk2r = w[k1 + 2];
      wk2i = w[k1 + 3];
      wtmp = 2 * wk2i;
      wk3r = wk1r - wtmp * wk1i;
      wk3i = wtmp * wk1r - wk1i;
      wk4r = 1 - wtmp * wk2i;
      wk4i = wtmp * wk2r;
      wtmp = 2 * wk4i;
      wk5r = wk3r - wtmp * wk1i;
      wk5i = wtmp * wk1r - wk3i;
      wk6r = wk2r - wtmp * wk2i;
      wk6i = wtmp * wk2r - wk2i;
      wk7r = wk1r - wtmp * wk3i;
      wk7i = wtmp * wk3r - wk1i;
      for (j = k; j < l + k; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        j4 = j3 + l;
        j5 = j4 + l;
        j6 = j5 + l;
        j7 = j6 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        y0r = x0r + x2r;
        y0i = x0i + x2i;
        y2r = x0r - x2r;
        y2i = x0i - x2i;
        y1r = x1r - x3i;
        y1i = x1i + x3r;
        y3r = x1r + x3i;
        y3i = x1i - x3r;
        x0r = a[j4] + a[j5];
        x0i = a[j4 + 1] + a[j5 + 1];
        x1r = a[j4] - a[j5];
        x1i = a[j4 + 1] - a[j5 + 1];
        x2r = a[j6] + a[j7];
        x2i = a[j6 + 1] + a[j7 + 1];
        x3r = a[j6] - a[j7];
        x3i = a[j6 + 1] - a[j7 + 1];
        y4r = x0r + x2r;
        y4i = x0i + x2i;
        y6r = x0r - x2r;
        y6i = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        x2r = x1r + x3i;
        x2i = x1i - x3r;
        y5r = wn4r * (x0r - x0i);
        y5i = wn4r * (x0r + x0i);
        y7r = wn4r * (x2r - x2i);
        y7i = wn4r * (x2r + x2i);
        x0r = y1r + y5r;
        x0i = y1i + y5i;
        a[j1] = wk1r * x0r - wk1i * x0i;
        a[j1 + 1] = wk1r * x0i + wk1i * x0r;
        x0r = y1r - y5r;
        x0i = y1i - y5i;
        a[j5] = wk5r * x0r - wk5i * x0i;
        a[j5 + 1] = wk5r * x0i + wk5i * x0r;
        x0r = y3r - y7i;
        x0i = y3i + y7r;
        a[j3] = wk3r * x0r - wk3i * x0i;
        a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        x0r = y3r + y7i;
        x0i = y3i - y7r;
        a[j7] = wk7r * x0r - wk7i * x0i;
        a[j7 + 1] = wk7r * x0i + wk7i * x0r;
        a[j] = y0r + y4r;
        a[j + 1] = y0i + y4i;
        x0r = y0r - y4r;
        x0i = y0i - y4i;
        a[j4] = wk4r * x0r - wk4i * x0i;
        a[j4 + 1] = wk4r * x0i + wk4i * x0r;
        x0r = y2r - y6i;
        x0i = y2i + y6r;
        a[j2] = wk2r * x0r - wk2i * x0i;
        a[j2 + 1] = wk2r * x0i + wk2i * x0r;
        x0r = y2r + y6i;
        x0i = y2i - y6r;
        a[j6] = wk6r * x0r - wk6i * x0i;
        a[j6 + 1] = wk6r * x0i + wk6i * x0r;
      }
    }
  }
}


void rftfsub(int n, float *a, int nc, float *c) {
  int j, k, kk, ks, m;
  float wkr, wki, xr, xi, yr, yi;

  m = n >> 1;
  ks = 2 * nc / m;
  kk = 0;
  for (j = 2; j < m; j += 2) {
    k = n - j;
    kk += ks;
    wkr = static_cast<float>(0.5 - c[nc - kk]);
    wki = c[kk];
    xr = a[j] - a[k];
    xi = a[j + 1] + a[k + 1];
    yr = wkr * xr - wki * xi;
    yi = wkr * xi + wki * xr;
    a[j] -= yr;
    a[j + 1] -= yi;
    a[k] += yr;
    a[k + 1] -= yi;
  }
}


void rftbsub(int n, float *a, int nc, float *c) {
  int j, k, kk, ks, m;
  float wkr, wki, xr, xi, yr, yi;

  a[1] = -a[1];
  m = n >> 1;
  ks = 2 * nc / m;
  kk = 0;
  for (j = 2; j < m; j += 2) {
    k = n - j;
    kk += ks;
    wkr = 0.5f - c[nc - kk];
    wki = c[kk];
    xr = a[j] - a[k];
    xi = a[j + 1] + a[k + 1];
    yr = wkr * xr + wki * xi;
    yi = wkr * xi - wki * xr;
    a[j] -= yr;
    a[j + 1] = yi - a[j + 1];
    a[k] += yr;
    a[k + 1] = yi - a[k + 1];
  }
  a[m + 1] = -a[m + 1];
}


void dctsub(int n, float *a, int nc, float *c) {
  int j, k, kk, ks, m;
  float wkr, wki, xr;

  m = n >> 1;
  ks = nc / n;
  kk = 0;
  for (j = 1; j < m; j++) {
    k = n - j;
    kk += ks;
    wkr = c[kk] - c[nc - kk];
    wki = c[kk] + c[nc - kk];
    xr = wki * a[j] - wkr * a[k];
    a[j] = wkr * a[j] + wki * a[k];
    a[k] = xr;
  }
  a[m] *= c[0];
}


void dstsub(int n, float *a, int nc, float *c) {
  int j, k, kk, ks, m;
  float wkr, wki, xr;

  m = n >> 1;
  ks = nc / n;
  kk = 0;
  for (j = 1; j < m; j++) {
    k = n - j;
    kk += ks;
    wkr = c[kk] - c[nc - kk];
    wki = c[kk] + c[nc - kk];
    xr = wki * a[k] - wkr * a[j];
    a[k] = wkr * a[k] + wki * a[j];
    a[j] = xr;
  }
  a[m] *= c[0];
}


void fft8g_end() {}
