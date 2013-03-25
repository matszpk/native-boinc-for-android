
// Copyright (c) 1999-2006 Regents of the University of California
//
// FFTW: Copyright (c) 2003,2006 Matteo Frigo
//       Copyright (c) 2003,2006 Massachusets Institute of Technology
//
// fft8g.[cpp,h]: Copyright (c) 1995-2001 Takya Ooura

// This program is free software; you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the 
// Free Software Foundation; either version 2, or (at your option) any later
// version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions
// as an alternative to FFTW and distribute a linked executable and 
// source code.  You must obey the GNU General Public License in all 
// respects for all of the code used other than the FFT library itself.  
// Any modification required to support these libraries must be distributed 
// under the terms of this license.  If you modify this program, you may extend 
// this exception to your version of the program, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.  Please be aware that FFTW is not covered by this exception, 
// therefore you may not use FFTW in any derivative work so modified without 
// permission of the authors of FFTW.
//
// $Id: analyzeFuncs_vector.cpp,v 1.1.2.29 2007/08/16 10:13:56 charlief Exp $

#include "sah_config.h" 

#ifdef __APPLE_CC__
#define _CPP_CMATH  // Block inclusion of <cmath> which undefines isnan() (for using GCC 3 on OS X)
#define _GLIBCXX_CMATH  // Block inclusion of <cmath> which undefines isnan() (for using GCC 4 on OS X)
#endif
#ifdef _WIN32
#define uint32_t unsigned long
#endif

#include <csignal>
#include <cstdlib>
#include <math.h>
#include <signal.h>
#include <setjmp.h>
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_FLOATINGPOINT_H
#include <floatingpoint.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "util.h"
#include "s_util.h"
#include "boinc_api.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif
#include "diagnostics.h"

#include "sighandler.h"
#include "analyzeFuncs.h"
#include "analyzeFuncs_vector.h"

#include "hires_timer.h"

#include "chirpfft.h"
#include "analyzePoT.h"
#include "pulsefind.h"
#ifdef USE_ASMLIB
#include "asmlib.h"
#endif

#ifdef __APPLE_CC__
#include <sys/sysctl.h>
#endif


#ifdef HAVE___ISNAN
#define isnotnan(x) (!__isnan(x))
#elif defined(HAVE__ISNAN)
#define isnotnan(x) (!_isnan(x))
#elif defined(HAVE_ISNAN)
#define isnotnan(x) (!isnan(x))
#else
#define isnotnan(x) ((x) == (x))
#endif

// Bit patterns to compare host capabilities and what SIMD capability a routine needs
#define BA_ANY     0x00000001              // any CPU OK
#define BA_MMX     0x00000002
#define BA_SSE     0x00000004
#define BA_SSE2    0x00000008
#define BA_SSE3    0x00000010
#define BA_3Dnow   0x00000020
#define BA_3DnowP  0x00000040
#define BA_MMX_P   0x00000080
#define BA_ALTVC   0x00010000
#define BA_NEON    0x00100000

#ifdef HAVE_ARMOPT
extern "C" {
extern int vfp_ChirpData(sah_complex* cx_DataArray, sah_complex* cx_ChirpDataArray,
    int chirp_rate_ind, double chirp_rate, int  ul_NumDataPoints, double sample_rate);
extern int neon_ChirpData(sah_complex* cx_DataArray, sah_complex* cx_ChirpDataArray,
    int chirp_rate_ind, double chirp_rate, int  ul_NumDataPoints, double sample_rate);

extern int vfp_GetPowerSpectrum(sah_complex* FreqData, float* PowerSpectrum,
                int NumDataPoints);
extern int neon_GetPowerSpectrum(sah_complex* FreqData, float* PowerSpectrum,
                int NumDataPoints);

extern FoldSet vfpFoldMain;
extern FoldSet neonFoldMain;
}
#endif

uint32_t CPUCaps = BA_ANY;

/**********************
 */
void SetCapabilities(void) {

#if defined(__APPLE_CC__)
  // OS X assumes MMX, SSE and SSE2 are present on Intel processors
  // SSE3 and Altivec are optional
#if defined(__i386__) || defined(__x86_64__)
  CPUCaps |= (BA_MMX|BA_SSE|BA_SSE2);
  int sse3flag=0;
  size_t length=sizeof(sse3flag);
  int error=sysctlbyname("hw.optional.sse3",&sse3flag,&length,NULL,0);
  if (sse3flag && !error) CPUCaps |= BA_SSE3;
#else  // PowerPC
  int altivecflag=0;
  size_t length=sizeof(altivecflag);
  int error=sysctlbyname("hw.optional.altivec",&altivecflag,&length,NULL,0);
  if (altivecflag && !error) CPUCaps |= BA_ALTVC;
#endif

#elif defined( USE_BHCPUID )
  CPU_INFO theCPU;

  if (theCPU.mmx()) CPUCaps |= BA_MMX;
  if (theCPU.sse()) CPUCaps |= BA_SSE;
  if (theCPU.sse2()) CPUCaps |= BA_SSE2;
  if (theCPU.sse3()) CPUCaps |= BA_SSE3;
  if (theCPU._3Dnow()) CPUCaps |= BA_3Dnow;
  if (theCPU._3DnowPlus()) CPUCaps |= BA_3DnowP;
  if (theCPU.mmxPlus()) CPUCaps |= BA_MMX_P;

#elif defined( USE_ASMLIB )
  int dp = DetectProcessor();
  if (dp & 0x00800000)  CPUCaps |= BA_MMX;
  if ((dp & 0x02000800) == 0x02000800)  CPUCaps |= BA_SSE;
  if ((dp & 0x04000800) == 0x04000800)  CPUCaps |= BA_SSE2;
  if ((dp & 0x08000800) == 0x08000800)  CPUCaps |= BA_SSE3;
  if (dp & 0x80000000)  CPUCaps |= BA_3Dnow;
  if (dp & 0x40000000)  CPUCaps |= BA_3DnowP;
  if (dp & 0x20000000)  CPUCaps |= BA_MMX_P;

#elif defined(__i386__) || defined (__x86_64__)
  /* we're goping to rely on signal handling to keep us out of trouble */
  CPUCaps |= (BA_MMX | BA_SSE | BA_SSE2 | BA_SSE3 | BA_3Dnow | BA_3DnowP | BA_MMX_P );

#elif defined(USE_ALTIVEC)
  CPUCaps |= BA_ALTVC;
#elif ANDROID
#  ifndef ONLY_VFP
  FILE* file = fopen("/proc/cpuinfo","rb");

  bool use_neon = false;
  char line[257];

  while ((fgets(line,256,file)) != NULL)
  {
    if (strstr(line, "Features\t: ") != line)
        continue;
    if (strstr(line,"vfpv3d16")!=NULL)
        use_neon = false;  // because vfpv3 uses 32 dp registers
    else if (strstr(line,"neon")!=NULL)
        use_neon = true;
    else if (strstr(line,"vfp")!=NULL)
        use_neon = false;
  }
  fclose(file);
  if (use_neon)
      CPUCaps |= BA_NEON;
#  endif
#endif
}


struct BLStb {
  BaseLineSmooth_func func;
  int ba;
  const char * const nom;
};

BLStb BaseLineSmoothFuncs[]={
      v_BaseLineSmooth, BA_ANY, "v_BaseLineSmooth",
      };

struct GPStb {
  GetPowerSpectrum_func func;
  int ba;
  const char * const nom;
};

GPStb GetPowerSpectrumFuncs[]={
    v_GetPowerSpectrum, BA_ANY, "v_GetPowerSpectrum",
#ifdef USE_ALTIVEC
    v_vGetPowerSpectrum, BA_ALTVC, "v_vGetPowerSpectrum", 
    v_vGetPowerSpectrumG4, BA_ALTVC,"v_vGetPowerSpectrumG4",
#endif
#ifdef USE_SSE 
    v_vGetPowerSpectrum, BA_SSE, "v_vGetPowerSpectrum",         
    v_vGetPowerSpectrum2, BA_SSE, "v_vGetPowerSpectrum2",        
    v_vGetPowerSpectrumUnrolled, BA_SSE, "v_vGetPowerSpectrumUnrolled", 
    v_vGetPowerSpectrumUnrolled2, BA_SSE, "v_vGetPowerSpectrumUnrolled2",
#endif
    };

struct CDtb {
  ChirpData_func func;
  int ba;
  const char * const nom;
};

CDtb ChirpDataFuncs[]={
    v_ChirpData, BA_ANY, "v_ChirpData",
    fpu_ChirpData, BA_ANY, "fpu_ChirpData",
#ifdef USE_SSE2
    v_vChirpData_x86_64, BA_SSE2, "v_vChirpData_x86_64",
#endif
#ifdef USE_ALTIVEC
    v_vChirpData, BA_ALTVC, "v_vChirpData",  
    v_vChirpDataG4, BA_ALTVC, "v_vChirpDataG4",
    v_vChirpDataG5, BA_ALTVC, "v_vChirpDataG5",
#endif
#ifdef  USE_SSE 
    sse1_ChirpData_ak, BA_SSE, "sse1_ChirpData_ak",
    sse2_ChirpData_ak, BA_SSE2, "sse2_ChirpData_ak",
    sse3_ChirpData_ak, BA_SSE3, "sse3_ChirpData_ak",
#endif
    };


struct TPtb {
  Transpose_func func;
  int ba;
  const char * const nom;
};

TPtb TransposeFuncs[]={
    v_Transpose, BA_ANY, "v_Transpose", 
    v_Transpose2, BA_ANY, "v_Transpose2",
    v_Transpose4, BA_ANY, "v_Transpose4",
    v_Transpose8, BA_ANY, "v_Transpose8",
#ifdef USE_ALTIVEC
    v_vTranspose, BA_ALTVC, "v_vTranspose",
#endif
#ifdef USE_SSE 
    v_pfTranspose2, BA_SSE, "v_pfTranspose2",      
    v_pfTranspose4, BA_SSE, "v_pfTranspose4",      
    v_pfTranspose8, BA_SSE, "v_pfTranspose8",      
    v_vTranspose4, BA_SSE, "v_vTranspose4",       
    v_vTranspose4np, BA_SSE, "v_vTranspose4np",     
    v_vTranspose4ntw, BA_SSE, "v_vTranspose4ntw",    
    v_vTranspose4x8ntw, BA_SSE, "v_vTranspose4x8ntw",  
    v_vTranspose4x16ntw, BA_SSE, "v_vTranspose4x16ntw", 
    v_vpfTranspose8x4ntw, BA_SSE, "v_vpfTranspose8x4ntw",
#endif
    };

struct FolSub {
  FoldSet *fsp;
  int      ba;
};

FolSub FoldSubs[] = {
  &swifold,      BA_ANY,
#ifdef USE_ALTIVEC
  &AKavfold,     BA_ALTVC,
#endif
#ifdef USE_SSE 
#if 0
// sse_ben_fold doesn't seem to work on PIII and Athlon.
// Need to debug later
  &sse_ben_fold, BA_SSE,
#endif
  &AKSSEfold,    BA_SSE,
  &BHSSEfold,    BA_SSE,
#endif
};


static bool do_print;

bool TestBoincSignalHandling() {
#ifdef USE_ASMLIB
    return true;
#endif
    install_sighandler();
    FORCE_FRAME_POINTER;
    if (setjmp(jb)) {
        uninstall_sighandler();
        return true;
    } else {
        // Try some illegal instructions to check the signal handling.
#ifdef __GNUC__
#if defined(__APPLE__) 
        uninstall_sighandler();
        return true;
#elif defined(__x86_64__)
	__asm__ ("movq %cr4,%rax");
#elif defined(__i386__) 
        __asm__ ("movl %cr4,%eax");
#elif defined(__ppc__) || defined(__sparc__)
__asm__ (".long %0": : "i" (0xfeedface) );
#endif
#elif defined(_MSC_VER) && ( defined(_M_IX86) || defined(_M_X64) )
#ifndef _NDEBUG
        uninstall_sighandler();
        return true;
#else
		__asm {
                  __asm __emit 00fh
		  __asm __emit 020h
		  __asm __emit 0e0h
		}
#endif
#endif
    }
    uninstall_sighandler();
    return false;
}


BaseLineSmooth_func ChooseBaseLineSmooth() {
    hires_timer timer;
    BaseLineSmooth_func baseline_smooth;
    int i,j,rv,k = sizeof(BaseLineSmoothFuncs)/sizeof(BLStb);
    int NumDataPoints=128*1024;
    double speed=1e+6,timing,accuracy;
    int best;
    double best_timing, best_accuracy;

    if (k == 1) {
      if (do_print) fprintf(stderr,"%30s (no other)%s\n",
                                    BaseLineSmoothFuncs[0].nom,
                                    verbose ? "\n": "");
      return v_BaseLineSmooth;
    }
    sah_complex *indata=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);
    sah_complex *outdata=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);
    sah_complex *save=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);

    FORCE_FRAME_POINTER;
    if (!indata || !outdata || !save) {
        if (indata)
            free_a(indata);
        if (outdata)
            free_a(outdata);
        if (save)
            free_a(save);
        return v_BaseLineSmooth;
    }
    for (i=0;i<NumDataPoints;i++) {
        indata[i][0]=((rand()&RAND_MAX)>(RAND_MAX/2))?-1:1;
        indata[i][1]=((rand()&RAND_MAX)>(RAND_MAX/2))?-1:1;
    }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
    install_sighandler();
    for (i=0;i<k;i++) {
        if (!setjmp(jb)) {
#else
    for (i=0;i<k;i++) {
#endif
    if (!(CPUCaps & BaseLineSmoothFuncs[i].ba)) continue;

            j=0;
            timing=0;
            while ((j<40) && ((j<4) || (timing<(3*timer.resolution())))) {
                memcpy(outdata,indata,NumDataPoints*sizeof(sah_complex));
                timer.start();
                rv=BaseLineSmoothFuncs[i].func(outdata,NumDataPoints,8192,32768);
                timing+=timer.stop();
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
                if (rv) longjmp(jb,1);
#else
                if (rv) break;
#endif
                j++;
            }
            if (rv) continue;
            timing/=j;
            if (i==0) {
                accuracy=0;
                memcpy(save,outdata,NumDataPoints*sizeof(sah_complex));
            } else {
                accuracy=0;
                for (j=0;j<NumDataPoints;j++) {
                    accuracy+=pow(save[j][0]-outdata[j][0],2);
                    accuracy+=pow(save[j][1]-outdata[j][1],2);
                }
            }
            if (verbose) 
                fprintf(stderr,"%30s %7.5f %7.5f  test\n",BaseLineSmoothFuncs[i].nom,timing,accuracy);
            if ((timing<speed) && isnotnan(accuracy) && (accuracy<1e-5)) {
                speed=timing;
                best=i;
                best_timing=timing;
                best_accuracy=accuracy;
                baseline_smooth=BaseLineSmoothFuncs[i].func;
            }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
        } else {
            reinstall_sighandler();
        }
    }
    uninstall_sighandler();
#else
    }
#endif
    free_a(indata);
    free_a(outdata);
    free_a(save);
    if (do_print)
      fprintf(stderr,"%30s %7.5f %7.5f %s\n",
             BaseLineSmoothFuncs[best].nom,
             best_timing,
             best_accuracy,
             verbose ? " choice\n": "");
    return baseline_smooth;
}

GetPowerSpectrum_func ChooseGetPowerSpectrum() {
    if (default_functions_flag) {
      if (do_print)
        fprintf(stderr,"%30s (default)\n",GetPowerSpectrumFuncs[0].nom);
      return GetPowerSpectrumFuncs[0].func;
    }  // else
    hires_timer timer;
    GetPowerSpectrum_func get_power_spectrum;
    int i,j,rv;
    double speed=1e+6,timing,mintime,onetime,accuracy;
    int NumDataPoints=128*1024;
    int best;
    double best_timing, best_accuracy;
    sah_complex *indata=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);
    float *outdata=(float *)malloc_a(NumDataPoints*sizeof(float),MEM_ALIGN);
    float *save=(float *)malloc_a(NumDataPoints*sizeof(float),MEM_ALIGN);

    FORCE_FRAME_POINTER;
    if (!indata || !outdata || !save) {
        if (indata)
            free_a(indata);
        if (outdata)
            free_a(outdata);
        if (save)
            free_a(save);
        return v_GetPowerSpectrum;
    }
    for (i=0;i<NumDataPoints;i++) {
        indata[i][0]=(float)((rand()&RAND_MAX)-RAND_MAX/2)/RAND_MAX;
        indata[i][1]=(float)((rand()&RAND_MAX)-RAND_MAX/2)/RAND_MAX;
    }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
    install_sighandler();
    for (i=0;(i*sizeof(GPStb))<sizeof(GetPowerSpectrumFuncs);i++) {
        if (!setjmp(jb)) {
#else
    for (i=0;(i*sizeof(GPStb))<sizeof(GetPowerSpectrumFuncs);i++) {
#endif
    if (!(CPUCaps & GetPowerSpectrumFuncs[i].ba)) continue;
            j=0;
            timing=0;
            mintime=1e6;
            while ((j<100) && ((j<10) || (timing<(3*timer.resolution())))) {
                memset(outdata,0,NumDataPoints*sizeof(float));
                timer.start();
                rv=GetPowerSpectrumFuncs[i].func(indata,outdata,NumDataPoints);
                onetime=timer.stop();
                timing+=onetime;
                if (onetime<mintime) mintime=onetime;
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
                if (rv) longjmp(jb,1);
#else
                if (rv) break;
#endif
                j++;
            }
            if (rv) continue;
            timing/=j;
            timing = (timing+mintime)/2;
            if (i==0) {
                accuracy=0;
                memcpy(save,outdata,NumDataPoints*sizeof(float));
            } else {
                accuracy=0;
                for (j=0;j<NumDataPoints;j++) {
                    accuracy+=pow(save[j]-outdata[j],2);
                }
                accuracy=sqrt(accuracy);
            }
            if (verbose) fprintf(stderr,"%30s %7.5f %7.5f  test\n",GetPowerSpectrumFuncs[i].nom,timing,accuracy);
            if ((timing<speed) && isnotnan(accuracy) && (accuracy<1e-5)) {
                speed=timing;
                best=i;
                best_timing=timing;
                best_accuracy=accuracy;
                get_power_spectrum=GetPowerSpectrumFuncs[i].func;
            }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
        } else {
            reinstall_sighandler();
        }
    }
    uninstall_sighandler();
#else
    }
#endif
    free_a(indata);
    free_a(outdata);
    free_a(save);
    if (do_print)
      fprintf(stderr,"%30s %7.5f %7.5f %s\n",
                      GetPowerSpectrumFuncs[best].nom,
                      best_timing,
                      best_accuracy,
                      verbose ? " choice\n": "");
    return get_power_spectrum;
}

ChirpData_func ChooseChirpData() {
    if (default_functions_flag) {
      if (do_print)
        fprintf(stderr,"%30s (default)\n",ChirpDataFuncs[0].nom);
      return ChirpDataFuncs[0].func;
    }  // else
    hires_timer timer;
    ChirpData_func chirp_data;
    int i,j,rv,k = sizeof(ChirpDataFuncs)/sizeof(CDtb);
    double speed=1e+6,timing,accuracy;
    int NumDataPoints=1024*1024;
    int best;
    int jj=0;
    double best_timing, best_accuracy;

    if (k == 1) {
      if (do_print) fprintf(stderr,"%30s  (no other)\n",ChirpDataFuncs[0].nom);
      return v_ChirpData;
    }
    sah_complex *indata=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);
    sah_complex *outdata=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);
    sah_complex *save=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);
    sah_complex *test=(sah_complex *)malloc_a(NumDataPoints*sizeof(sah_complex),MEM_ALIGN);

    FORCE_FRAME_POINTER;
    if (!indata || !outdata || !save || !test) {
        if (indata)
            free_a(indata);
        if (outdata)
            free_a(outdata);
        if (save)
            free_a(save);
        if (test)
            free_a(test);
        fprintf(stderr,"Memory allocation failed in ChooseChirp\n");
        return v_ChirpData;
    }
    for (i=0;i<NumDataPoints;i++) {
        indata[i][0]=((rand()&RAND_MAX)>(RAND_MAX/2))?-1:1;
        indata[i][1]=((rand()&RAND_MAX)>(RAND_MAX/2))?-1:1;
    }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
    install_sighandler();
    for (i=0;i<k;i++) {
        if (!setjmp(jb)) {
#else
    for (i=0;i<k;i++) {
#endif
    if (!(CPUCaps & ChirpDataFuncs[i].ba)) continue;
            j=0;
            timing=0;
            int ind=TESTCHIRPIND;
            while ((j<100) && ((j<10) || (timing<(3*timer.resolution())))) {
                memset(outdata,0,NumDataPoints*sizeof(sah_complex));
                timer.start();
                rv=ChirpDataFuncs[i].func(indata,outdata,ind,MinChirpStep*ind,NumDataPoints,2.5e+6/256.0);
                timing+=timer.stop();
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
                if (rv) longjmp(jb,1);
#else
                if (rv) break;
#endif
                if (j==1) memcpy(test,outdata,NumDataPoints*sizeof(sah_complex));
                if (ind>0) {
                    ind*=-1;
                } else {
                    ind=-1*(ind-2);
                }
                j++;
            }
            if (rv) continue;
            jj=j;
            timing/=j;
            if (i==0) {
                accuracy=0;
                memcpy(save,test,NumDataPoints*sizeof(sah_complex));
            } else {
                accuracy=0;
                for (j=0;j<NumDataPoints;j++) {
                    accuracy+=pow(save[j][0]-test[j][0],2);
                    accuracy+=pow(save[j][1]-test[j][1],2);
                }
                accuracy=sqrt(accuracy)/(NumDataPoints*2);
            }
            if (verbose) fprintf(stderr,"%30s %7.5f %7.5f  test\n",ChirpDataFuncs[i].nom,timing,accuracy);
            if ((timing<speed) && isnotnan(accuracy) && (accuracy<5e-3)) {
                speed=timing;
                best=i;
                best_timing=timing;
                best_accuracy=accuracy;
                chirp_data=ChirpDataFuncs[i].func;
            }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
        } else {
            reinstall_sighandler();
        }
    }
    uninstall_sighandler();
#else
    }
#endif
    free_a(indata);
    free_a(outdata);
    free_a(save);
    free_a(test);
    if (do_print)
        fprintf(stderr,"%30s %7.5f %7.5f %s\n",
                       ChirpDataFuncs[best].nom,
                       best_timing,
                       best_accuracy,
                       verbose ? " choice\n": "");    
    return chirp_data;
}

Transpose_func ChooseTranspose() {
    if (default_functions_flag) {
      if (do_print)
        fprintf(stderr,"%30s (default)\n",TransposeFuncs[0].nom);
      return TransposeFuncs[0].func;
    }  // else
    hires_timer timer;
    Transpose_func transpose;
    int i,j,rv;
    double speed=1e+6,timing,mintime,onetime,accuracy;
    int NumDataPoints=1024*1024;
    int best;
    double best_timing, best_accuracy;    
    float *indata=(float *)malloc_a(NumDataPoints*sizeof(float),MEM_ALIGN);
    float *outdata=(float *)malloc_a(NumDataPoints*sizeof(float),MEM_ALIGN);
    float *save=(float *)malloc_a(NumDataPoints*sizeof(float),MEM_ALIGN);

    FORCE_FRAME_POINTER;
    if (!indata || !outdata || !save) {
        if (indata)
            free_a(indata);
        if (outdata)
            free_a(outdata);
        if (save)
            free_a(save);
        return v_Transpose;
    }
    for (i=0;i<NumDataPoints;i++) {
        indata[i]=rand();
    }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
    install_sighandler();
    for (i=0;(i*sizeof(TPtb))<sizeof(TransposeFuncs);i++) {
        if (!setjmp(jb)) {
#else
    for (i=0;(i*sizeof(TPtb))<sizeof(TransposeFuncs);i++) {
#endif
    if (!(CPUCaps & TransposeFuncs[i].ba)) continue;
            j=0;
            timing=0;
            mintime=1e6;
            int ind=0;
            while ((j<100) && ((j<10) || (timing<(3*timer.resolution())))) {
                memset(outdata,0,NumDataPoints*sizeof(float));
                timer.start();
                rv=TransposeFuncs[i].func(16384,64,indata,outdata);
                onetime=timer.stop();
                timing+=onetime;
                if (onetime<mintime) mintime=onetime;
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
                if (rv) longjmp(jb,1);
#else
                if (rv) break;
#endif
                j++;
            }
            if (rv) continue;
            timing/=j;
            timing = (timing+mintime)/2;
            if (i==0) {
                accuracy=0;
                memcpy(save,outdata,NumDataPoints*sizeof(float));
            } else {
                accuracy=0;
                for (j=0;j<NumDataPoints;j++) {
                    accuracy+=pow(save[j]-outdata[j],2);
                }
                accuracy=sqrt(accuracy);
            }
            if (verbose) fprintf(stderr,"%30s %7.5f %7.5f  test\n",TransposeFuncs[i].nom,timing,accuracy);
            if ((timing<speed) && isnotnan(accuracy) && (accuracy<1e-6)) {
                speed=timing;
                best=i;
                best_timing=timing;
                best_accuracy=accuracy;
                transpose=TransposeFuncs[i].func;
            }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
        } else {
            reinstall_sighandler();
        }
    }
    uninstall_sighandler();
#else
    }
#endif
    free_a(indata);
    free_a(outdata);
    free_a(save);
    if (do_print)
        fprintf(stderr,"%30s %7.5f %7.5f %s\n",
                        TransposeFuncs[best].nom,
                        best_timing,
                        best_accuracy,
                        verbose ? " choice\n": "");    
    return transpose;
}

/********************************************
 *
 * Testing folding subroutines
 *
 */
// dummy folding routine
float dmyFld(float *ss[], struct PoTPlan *P) {return 1.0f;}

// tables to hold function pointers of set under test
sum_func fold3test[FOLDTBLEN] = {
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
};
sum_func fold4test[FOLDTBLEN] = {
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
};
sum_func fold5test[FOLDTBLEN] = {
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
};
sum_func fold2test[FOLDTBLEN] = {
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
};
sum_func fold2ALtest[FOLDTBLEN] = {
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
  dmyFld, dmyFld, dmyFld, dmyFld,
};

FoldSet TestFoldSet = {fold3test, fold4test, fold5test, fold2test, fold2ALtest, ""};

/**********************
 *
 * Generate preplanned sequence of folds
 *
 */
int planFoldTest(PoTPlan * PSeq, float *div, int FFTtbl[][5]) {
  float period;
  int i, iL, j, di, ndivs;
  int num_adds, num_adds_2;
  int FftLength, PulsePoTLen;
  int dbinoffs[32] = {0};
  int k = 0; // plan index

  for (iL = 0; iL < 32; iL++) {
    if ((FftLength = FFTtbl[iL][0]) == 0) continue;
    PulsePoTLen = FFTtbl[iL][4];

    for (i = 32, ndivs = 1; i <= PulsePoTLen; ndivs++, i *= 2);

    period = (float)((int)((PulsePoTLen * 2) / 3)) * 0.5f;
    for (i = 1; i < ndivs; i++, period *= 0.5f) {
      dbinoffs[i] = (((int)(period + 0.5f) + 
                      (MEM_ALIGN/sizeof(float)) - 1) &
                      -(MEM_ALIGN/sizeof(float))) + 
                      dbinoffs[i - 1];
    }

    int32_t PoTL = PulsePoTLen;

    //  Periods from PulsePoTLen/3 to PulsePoTLen/4, and power of 2 fractions of.
    //   then (len/4 to len/5) and finally (len/5 to len/6)
    //
    //  For quick testing we select about one from each range at FFT length 8, two at 16,
    //  four at 32, etc.
    //
    int32_t firstP, lastP, iP, iLim;
    for(num_adds = 3; num_adds <= 5; num_adds++) {
      float rnum_adds_minus1;
      float fP, fStep, fLim;
      iLim = FFTtbl[iL][num_adds - 2];
      switch(num_adds) {
        case 3: lastP = (PoTL * 2) / 3;  firstP = (PoTL * 1) / 2; rnum_adds_minus1 = 0.5f; break;
        case 4: lastP = (PoTL * 3) / 4;  firstP = (PoTL * 3) / 5; rnum_adds_minus1 = 1.0f / 3.0f; break;
        case 5: lastP = (PoTL * 4) / 5;  firstP = (PoTL * 4) / 6; rnum_adds_minus1 = 0.25f; break;
      }
      fStep = (float)(lastP - firstP) / (float)(iLim);
      fP = fabs((float)(lastP) - (fStep / 3.0f));

      for (iP = 0; iP < iLim; iP++, fP -= fStep) {
        PSeq[k].dest = div + dbinoffs[0]; // Output storage
        PSeq[k].di = di = (int)(fP * rnum_adds_minus1);
        PSeq[k].tmp0 = (int)(fP * 1.0f * rnum_adds_minus1 + 0.5f);
        PSeq[k].tmp1 = (int)(fP * 2.0f * rnum_adds_minus1 + 0.5f);

        switch(num_adds) {
          case 3:
            PSeq[k].fun_ptr = (di < FOLDTBLEN) ? fold3test[di] : fold3test[FOLDTBLEN-1];
            break;
          case 4:
            PSeq[k].tmp2 = (int)(fP * 3.0f * rnum_adds_minus1 + 0.5f);
            PSeq[k].fun_ptr = (di < FOLDTBLEN) ? fold4test[di] : fold4test[FOLDTBLEN-1];
            break;
          case 5:
            PSeq[k].tmp2 = (int)(fP * 3.0f * rnum_adds_minus1 + 0.5f);
            PSeq[k].tmp3 = (int)(fP * 4.0f * rnum_adds_minus1 + 0.5f);
            PSeq[k].fun_ptr = (di < FOLDTBLEN) ? fold5test[di] : fold5test[FOLDTBLEN-1];
            break;
        }
        // When built with DevC++/MinGW and signal handling enabled, the first calculation
        // of PSeq[0].tmp0 somehow produces an 0x80000000 value. This hack repeats the
        // calculation, which gets the right value.
        if (k == 0 && PSeq[k].tmp0 < 1) PSeq[k].tmp0 = (int)(fP * 1.0f * rnum_adds_minus1 + 0.5f);

        k++;                    // next plan
        num_adds_2 = 2 * num_adds;

        for (j = 1; j < ndivs ; j++) {
          PSeq[k].offset = dbinoffs[j - 1];
          PSeq[k].dest = div + dbinoffs[j];
          PSeq[k].tmp0 = di & 1;
          di /= 2;
          PSeq[k].tmp0 += di + PSeq[k].offset;
          PSeq[k].di = di;
          if (PSeq[k].tmp0 & 3) PSeq[k].fun_ptr  = (di < FOLDTBLEN) ? fold2test[di] : fold2test[FOLDTBLEN-1];
          else PSeq[k].fun_ptr  = (di < FOLDTBLEN) ? fold2ALtest[di] : fold2ALtest[FOLDTBLEN-1];

          k++;                    // next plan
          num_adds_2 *= 2;
        }  // for (j =
      } // for (iP = 
    } // for(num_adds =
  } // for(iL =
  PSeq[k].di = 0; // stop
/*
  for (int iDmp = 0; iDmp < 4; iDmp++) {
    fprintf(stderr,"di= %d dest= %d ptr= %d tmp0= %d tmp1= %d\n",
           PSeq[iDmp].di, PSeq[iDmp].dest, PSeq[iDmp].fun_ptr, PSeq[iDmp].tmp0, PSeq[iDmp].tmp1);
  }
*/
  return (k);
}


/**********************
 *
 * Test folding subroutine sets and choose best
 *
 */
int ChooseFoldSubs(ChirpFftPair_t * ChirpFftPairs, int num_cfft, int nsamples) {
  if (default_functions_flag) {
    if (do_print)
      fprintf(stderr,"%22s folding (default)\n",Foldmain.name);
    return 0;
  }  // else
  hires_timer timer;
  int i, iL, j, k, ndivs, NumPlans = 0, MaxPulsePoT = PoTInfo.PulseMax;
  double onetime, timing, best_timing, speed = 1e+6;
  double accuracy,  best_accuracy, errmax, dTmp = 1e+30;
  int best;
  int PoTLen, PulsePoTLen, Overlap, FFTtbl[32][5] = {0, 0, 0, 0, 0};
  double NumSamples = nsamples;
  float *SrcSel[2];

  // Get the count of chirp/fft pairs for each FFT length where Pulse finding
  // will be done.
  for (i = 0; i < num_cfft; i++) {
    if (ChirpFftPairs[i].PulseFind) {
      for (iL = 0, j = ChirpFftPairs[i].FftLen; j ; iL++, j >>= 1);
      FFTtbl[iL][0] = ChirpFftPairs[i].FftLen;
      FFTtbl[iL][2] += 1;
    }
  }

  // For testing, the chirp/fft table may just be a short list. If so, add the FFT
  // length to the counts to simulate distribution for a full table. (This also affects
  // very high angle range WUs, but Pulse folding is a small factor there so looser
  // testing will have very little effect.)
  if (num_cfft < 14) {
    for (iL = 0; iL < 32; iL++) FFTtbl[iL][2] += FFTtbl[iL][0];
  }

  // Scale the counts so the minimum is 1. Use the scaled value directly for fold
  // sequences starting with a fold by 4. Adjust the values for fold by 3 and
  // fold by 5 to get close to the 10, 9, 8 ratios. Add the PoT length values to
  // the table, and tot up the needed number of PoTPlans.
  for (iL = 0; iL < 32; iL++) {
    if ((FFTtbl[iL][0]) && ((double)FFTtbl[iL][2] < dTmp)) dTmp = (double)FFTtbl[iL][2];
  }
  for (iL = 0; iL < 32; iL++) {
    if (FFTtbl[iL][0]) {
      FFTtbl[iL][2] = (int)((double)FFTtbl[iL][2] / dTmp + 0.5);
      FFTtbl[iL][1] = (int)((double)FFTtbl[iL][2] * 10.0 / 9.0 + 0.5);
      FFTtbl[iL][3] = FFTtbl[iL][2] + FFTtbl[iL][2] - FFTtbl[iL][1];
      PoTLen = (int)(NumSamples / FFTtbl[iL][0] + 0.5);
      GetPulsePoTLen(PoTLen, &PulsePoTLen, &Overlap);
      FFTtbl[iL][4] = PulsePoTLen;
      for (i = 32, ndivs = 1; i <= PulsePoTLen; ndivs++, i *= 2);
      NumPlans += 3 * FFTtbl[iL][2] * ndivs;
    }
  }
  if (NumPlans == 0) return 0; // No pulse finding in this WU, abort test.

  // At some angle ranges the total number of test folds may be too few to get good
  // timing measurements. Scale up so there are at least 16K test folds.
  while (NumPlans < 16384) {
    for (iL = 0; iL < 32; iL++) {
      FFTtbl[iL][1] *= 2;
      FFTtbl[iL][2] *= 2;
      FFTtbl[iL][3] *= 2;
    }
    NumPlans *= 2;
  }

//  fprintf(stderr, "Calculated Preplans = %d\n", NumPlans);
//  NumPlans *= 2; // temporary safety measure


  // Now we know what we're doing - allocate memory in which to do it.
  PoTPlan *PlanBuf = (PoTPlan *)malloc_a((NumPlans + 1) * sizeof(PoTPlan), MEM_ALIGN);
  float *indata = (float *)malloc_a(MaxPulsePoT * sizeof(float), MEM_ALIGN);
  float *outdata = (float *)malloc_a(MaxPulsePoT * sizeof(float), MEM_ALIGN);
  float *maxdata = (float *)malloc_a(NumPlans * sizeof(float), MEM_ALIGN);
  float *save = (float *)malloc_a(NumPlans * sizeof(float), MEM_ALIGN);

  FORCE_FRAME_POINTER;
  if (!PlanBuf || !indata || !outdata || !maxdata || !save) {
    if (PlanBuf)
      free_a(PlanBuf);
    if (indata)
      free_a(indata);
    if (outdata)
      free_a(outdata);
    if (maxdata)
      free_a(maxdata);
    if (save)
      free_a(save);
    return 0;   // Can't test, make no change
  }

  SrcSel[0] = indata;
  SrcSel[1] = outdata;

  // Generate PowerSpectrum random data
  srand(11);
  for (i = 0; i < MaxPulsePoT; i++) {
    float fr1 = (float)(rand()) / RAND_MAX;
    float fr2 = (float)(rand()) / RAND_MAX;
    indata[i] = fr1 * fr1 + fr2 * fr2;
  }

#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
  install_sighandler();
  for (i=0;(i*sizeof(FolSub))<sizeof(FoldSubs);i++) {
    if (!setjmp(jb)) {
#else
  for (i = 0; (i * sizeof(FolSub)) < sizeof(FoldSubs); i++) {
#endif
      if (!(CPUCaps & FoldSubs[i].ba)) continue;
      j = 0;
      timing = 0;
      CopyFoldSet(&TestFoldSet, FoldSubs[i].fsp);
      int n = planFoldTest(PlanBuf, outdata, FFTtbl);
//    if (!i) fprintf(stderr, "Actual Preplans = %d\n", n);
      while ((j < 100) && ((j < 10) || ((j * timing) < (3 * timer.resolution())))) {
        memset(outdata, 0, MaxPulsePoT * sizeof(float)); 
        memset(maxdata, 0, NumPlans * sizeof(float)); 
        maxdata[0] = -1.234f;
        timer.start();
        for ( k = 0; PlanBuf[k].di; k++) {
          maxdata[k] = PlanBuf[k].fun_ptr(SrcSel, &PlanBuf[k]);
        }
        onetime = timer.stop();
        if (j) timing = std::min(onetime, timing);
        else timing = onetime;
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
        if (maxdata[0] < 0) longjmp(jb,1);
#else
        if (maxdata[0] < 0) break;
#endif
        j++;
      }
      accuracy = 0;
      errmax = 0;
      if (i == 0) {
        memcpy(save, maxdata, NumPlans * sizeof(float));
      } else {
        for (j = 0; j < NumPlans; j++) {
          // a zero max should never happen, but be safe...
          if (save[j]) {
            double relerr = fabs((save[j] - maxdata[j]) / save[j]);
            accuracy += relerr;
            if (relerr > errmax) errmax = relerr;
          }
        }
      }
      accuracy /= NumPlans;
      if (verbose) {
        fprintf(stderr, "%22s folding %7.5f %7.5f  test\n", FoldSubs[i].fsp->name, timing, accuracy);
//      if (errmax > 1e-5)
//        fprintf(stderr, "%22s folding errmax= %7.5f  test\n", FoldSubs[i].fsp->name, errmax);
//      fprintf(stderr, "12max: %g %g %g %g %g %g %g %g %g %g %g %g\n",
//              maxdata[0], maxdata[1], maxdata[2], maxdata[3], maxdata[4], maxdata[5],
//              maxdata[6], maxdata[7], maxdata[8], maxdata[9], maxdata[10], maxdata[11]);
      }
      if ((timing < speed) && isnotnan(accuracy) && (accuracy < 1e-6) && (errmax < 1e-4)) {
        speed = timing;
        best = i;
        best_timing = timing;
        best_accuracy = accuracy;
      }
#if !defined(USE_ASMLIB) && !defined(__APPLE_CC__)
    } else {
      reinstall_sighandler();
    }
  }
  uninstall_sighandler();
#else
  }
#endif
  free_a(PlanBuf);
  free_a(indata);
  free_a(outdata);
  free_a(maxdata);
  free_a(save);
  if (do_print)
    fprintf(stderr, "%22s folding %7.5f %7.5f %s\n",
                    FoldSubs[best].fsp->name,
                    best_timing,
                    best_accuracy,
                    verbose ? " choice\n": "");  
  CopyFoldSet(&Foldmain, FoldSubs[best].fsp);
  return 0;
}



void ChooseFunctions(BaseLineSmooth_func *baseline_smooth,
                     GetPowerSpectrum_func *get_power_spectrum,
                     ChirpData_func *chirp_data,
                     Transpose_func *transpose,
                     ChirpFftPair_t * ChirpFftPairs,
                     int num_cfft,
                     int nsamples,
                     bool print_choices) {
    do_print=print_choices;
    if (do_print) {
      fprintf(stderr,"Optimal function choices:\n");
      fprintf(stderr,"-----------------------------------------------------\n");
      fprintf(stderr,"%.30s %7s %7s\n","name","timing","error");
      fprintf(stderr,"-----------------------------------------------------\n");
    }
    SetCapabilities();
#ifdef ANDROID
    *baseline_smooth = v_BaseLineSmooth;
#ifdef HAVE_ARMOPT
    if ((CPUCaps & BA_NEON) != 0)
    {
        *get_power_spectrum = neon_GetPowerSpectrum;
        *chirp_data = neon_ChirpData;
        CopyFoldSet(&Foldmain, &neonFoldMain);
        fputs("Use CPU NEON routines\n",stderr);
    }
    else
    {
        *get_power_spectrum = vfp_GetPowerSpectrum;
        *chirp_data = vfp_ChirpData;
        CopyFoldSet(&Foldmain, &vfpFoldMain);
        fputs("Use CPU VFP routines\n",stderr);
    }
#else
    *get_power_spectrum=v_GetPowerSpectrum;
    *chirp_data=v_ChirpData;
#endif
    *transpose = v_Transpose4;
#else
    if (TestBoincSignalHandling()) {
        *baseline_smooth=ChooseBaseLineSmooth();
        *get_power_spectrum=ChooseGetPowerSpectrum();
        *chirp_data=ChooseChirpData();
        *transpose=ChooseTranspose();
        ChooseFoldSubs(ChirpFftPairs, num_cfft, nsamples);
    }
#endif

    if (do_print) {
      fflush(stderr);
    }

}


