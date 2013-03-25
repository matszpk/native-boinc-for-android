// This code is from the General Purpose FFT (Fast Fourier/Cosine/Sine Transform) Package.
// Copyright Takuya OOURA, 1996-2001 
// (Email: ooura@kurims.kyoto-u.ac.jp or ooura@mmm.t.u-tokyo.ac.jp) 
//
// You may use, copy, modify and distribute this code for any purpose 
// (include commercial use) and without fee. Please refer to this package 
// when you modify this code. 

// $Id: fft8g.h,v 1.3.2.1 2005/05/19 00:42:30 korpela Exp $
void cdft(int, int, sah_complex *, int *, float *);
void rdft(int, int, float *, int *, float *);
void ddct(int, int, float *, int *, float *);
void ddst(int, int, float *, int *, float *);
void dfct(int, float *, float *, int *, float *);
void dfst(int, float *, float *, int *, float *);
