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

// analyzeFuncs.C
// $Id: analyzeFuncs.cpp,v 1.34.2.44 2007/08/16 10:13:55 charlief Exp $
//

#define DO_SMOOTH

#if defined(USING_XCODE) || defined(_WIN32)
#include "version.h"
const char *BOINC_PACKAGE_STRING="libboinc: "BOINC_VERSION_STRING;
#else
#include "config.h"
const char *BOINC_PACKAGE_STRING="libboinc: "PACKAGE_STRING;
#endif

#undef PACKAGE_STRING
#undef PACKAGE
#undef PACKAGE_NAME
#undef PACKAGE_BUGREPORT
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include "sah_config.h"
const char *SAH_PACKAGE_STRING=CUSTOM_STRING;

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cmath>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <time.h>

#include "sincos.h"
#include "util.h"
#include "s_util.h"
#include "boinc_api.h"
#ifdef BOINC_APP_GRAPHICS
#include "sah_gfx_main.h"
#endif
#include "diagnostics.h"

// In order to use IPP, set -DUSE_IPP and one of -DUSE_SSE3, -DUSE_SSE2,
// -DUSE_SSE or nothing(generic),  IPP precedes FFTW, ooura // TMR
#if defined(USE_IPP)
#pragma message ("-----IPP-----")
#if defined(USE_SSE3)
#define T7 1
#pragma message ("-----sse3-----")
#include <ipp_t7.h>
#elif defined(USE_SSE2)
#define W7 1
#pragma message ("-----sse2-----")
#include <ipp_w7.h>
#elif defined(USE_SSE)
#define A6 1
#pragma message ("-----sse-----")
#include <ipp_a6.h>
#else
#pragma message ("-----mmx-----")
#include <ipp_px.h>
#endif // T7
#include <ipp.h>
#elif defined(USE_FFTWF)
#pragma message ("----FFTW----")
#include "fftw3.h"
#else
#pragma message ("----ooura----")
#include "fft8g.h"
#endif // USE_IPP

#if defined(USE_FFTS)
#pragma message ("----FFTS----")
#include "ffts/ffts.h"
#pragma message ("----FFTS_VFP_P----")
#include "ffts/ffts_vfp_p.h"
#endif

#ifdef HAVE_PFFFT
#include "pffft/pffft.h"
#endif

#include "seti.h"
#include "analyze.h"
#include "analyzeReport.h"
#include "gaussfit.h"
#include "spike.h"
#include "autocorr.h"
#include "malloc_a.h"
#include "analyzeFuncs.h"
#include "analyzePoT.h"
#include "chirpfft.h"
#include "worker.h"
#include "filesys.h"
#include "progress.h"

BaseLineSmooth_func BaseLineSmooth=v_BaseLineSmooth;
GetPowerSpectrum_func GetPowerSpectrum=v_GetPowerSpectrum;
ChirpData_func ChirpData=v_ChirpData;
Transpose_func Transpose=v_Transpose4;

#ifdef USE_IPP
static int MaxBufSize = 0;
static Ipp8u* FftBuf = NULL;
#endif // USE_IPP

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define INVALID_CHIRP 2e+20

ChirpFftPair_t* ChirpFftPairs = NULL;

double ProgressUnitSize;
double progress=0, remaining=1;

// These are used to calculate chirped signals
// TrigStep contains trigonometric functions for MinChirpStep over time
// CurrentTrig contains current trigonometric fuctions
//    Tetsuji "Maverick" Rai

// Trigonometric arrays
SinCosArray* TrigStep = NULL;     // trigonometric array of MinChirpStep
SinCosArray* CurrentTrig = NULL;  // current chirprate trigonometric array
int CurrentChirpRateInd;          // current chirprate index (absolute value)
double MinChirpStep=0.0;
bool use_transposed_pot;

void InitTrigArray(int, double, int, double);
void FreeTrigArray(void);
void CalcTrigArray (int len, int ChirpRateInd);

// The main analysis function.  Args:
// state pointer to data, # of points, starting chirp/fftlen
//  Must be called with unchirped data;
//  this function modifies (chirps) the data in place
// swi  parsed WU header

//#define DEBUG
#ifdef DEBUG
int icfft;  // for debug
#endif

#ifdef ANDROID
bool checkNEON()
{
#ifdef ONLY_VFP
    return false;
#else
    FILE* file = NULL;
    file = fopen("/proc/cpuinfo","rb");

    bool use_neon = false;
    char line[257];

    while ((fgets(line,256,file))!= NULL)
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
    return use_neon;
#endif
}
#endif

#ifdef USE_FFTS
/* true - use ffts (neon version) */
/* false - use ffts (vfp version) */
static bool useFFTSNEON = true;
static bool usePFFFT2 = false;
#endif

#ifdef HAVE_PFFFT
static float* pffft_work = NULL;
#endif

int seti_analyze (ANALYSIS_STATE& state) {
    sah_complex* DataIn = state.savedWUData;
    int NumDataPoints = state.npoints;
    sah_complex* ChirpedData = NULL;
    sah_complex* WorkData = NULL;
    float* PowerSpectrum = NULL;
    float* tPowerSpectrum; // Transposed power spectra if used.
    float* AutoCorrelation = NULL;
#if defined(ANDROID) && defined(USE_FFTS)
    useFFTSNEON = checkNEON();
#endif
        
#if defined(ANDROID) && defined(USE_FFTS)
    {
        long x = swi.analysis_cfg.bsmooth_chunk_size;
        for (; (x & 1L) == 0; x>>=1);
        if (x != 1L)
        {
            fputs("Use pffft because no 2**n\n",stderr);
            usePFFFT2 = true;
        }
    }
#endif

    use_transposed_pot= (!notranspose_flag) &&
                        ((app_init_data.host_info.m_nbytes != 0)  &&
                        (app_init_data.host_info.m_nbytes >= (double)(96*1024*1024)));
    int num_cfft                  = 0;
    float chirprate;
    int last_chirp_ind = - 1 << 20, chirprateind;

    double progress_diff, progress_in_cfft, cputime0=0;
    int retval=0;

    if (swi.analysis_cfg.credit_rate != 0) LOAD_STORE_ADJUSTMENT=swi.analysis_cfg.credit_rate;

#ifndef DEBUG
    int icfft;
#endif
    int NumFfts, ifft, fftlen;
    int CurrentSub;
    int FftNum, need_transpose;
    unsigned long bitfield=swi.analysis_cfg.analysis_fft_lengths;
    unsigned long FftLen;
    unsigned long ac_fft_len=swi.analysis_cfg.autocorr_fftlen;

#ifdef USE_IPP
    IppsFFTSpec_C_32fc* FftSpec[MAX_NUM_FFTS];
    int BufSize;

    ippStaticInit();   // initialization of IPP library
#elif defined(USE_FFTWF)
    // plan space for fftw
#ifndef USE_FFTS
    fftwf_plan analysis_plans[MAX_NUM_FFTS];
#endif
    fftwf_plan autocorr_plan;
#else
    // fields need by the ooura fft logic
    int * BitRevTab[MAX_NUM_FFTS];
    float * CoeffTab[MAX_NUM_FFTS];
#endif
#if defined(USE_FFTS)
    // plan space for ffts
    ffts_plan_t *analysis_plans[MAX_NUM_FFTS];
#endif
#ifdef USE_FFTS_VFP_P
    vfp_ffts_plan_t *analysis_plans_vfp[MAX_NUM_FFTS];
#endif

    // Allocate data array and work area arrays.
    ChirpedData = state.data;
    PowerSpectrum = (float*) calloc_a(NumDataPoints, sizeof(float), MEM_ALIGN);
    if (PowerSpectrum == NULL) SETIERROR(MALLOC_FAILED, "PowerSpectrum == NULL");
    if (use_transposed_pot) {
        tPowerSpectrum = (float*) calloc_a(NumDataPoints, sizeof(float), MEM_ALIGN);
        if (tPowerSpectrum == NULL) SETIERROR(MALLOC_FAILED, "tPowerSpectrum == NULL");
    } else {
        tPowerSpectrum=PowerSpectrum;
    }
    AutoCorrelation = (float*)calloc_a(ac_fft_len, sizeof(float), MEM_ALIGN);
    if (AutoCorrelation == NULL) SETIERROR(MALLOC_FAILED, "AutoCorrelation == NULL");

    // boinc_worker_timer();
    FftNum=0;
    FftLen=1;



#ifdef USE_FFTWF
    double sz;
    FILE *wisdom;
    if ((wisdom=boinc_fopen("wisdom.sah","r"))) {
        char *wiz=(char *)calloc_a(1024,64,MEM_ALIGN);
        int n=0;
        while (wiz && n<64*1024 && !feof(wisdom)) {
            n+=fread(wiz+n,1,80,wisdom);
        }
        fftwf_import_wisdom_from_string(wiz);
        free_a(wiz);
        fclose(wisdom);
    }
#endif


#ifdef BOINC_APP_GRAPHICS
    if (sah_graphics) strcpy(sah_graphics->status, "Generating FFT Coefficients");
#endif
#ifdef HAVE_PFFFT
    unsigned long maxFftLen = swi.analysis_cfg.bsmooth_chunk_size; // NumPointsInChunk
#endif
    while (bitfield != 0) {
        if (bitfield & 1) {
            swi.analysis_fft_lengths[FftNum]=FftLen;
#ifdef USE_IPP
            int order = 0;
            for (int tmp = FftLen; !(tmp & 1); order++) tmp >>= 1;
            if (ippsFFTInitAlloc_C_32fc(&FftSpec[FftNum], order,
                                        IPP_FFT_NODIV_BY_ANY, ippAlgHintFast)) {
                SETIERROR (MALLOC_FAILED, "ippsFFTInitAlloc failed");
            }
#elif defined(USE_FFTS)
#ifdef USE_FFTS_VFP_P
            if (!useFFTSNEON)
                analysis_plans_vfp[FftNum] = vfp_ffts_init_1d(FftLen, 1);
            else
#endif
            analysis_plans[FftNum] = ffts_init_1d(FftLen, 1);
#elif !defined(USE_FFTWF)
            // See docs in fft8g.C for sizing guidelines for BitRevTab and CoeffTab.
            BitRevTab[FftNum] = (int*) calloc_a(3+(int)sqrt((float)swi.analysis_fft_lengths[FftNum]), sizeof(int), MEM_ALIGN);
            if (BitRevTab[FftNum] == NULL)  SETIERROR(MALLOC_FAILED, "BitRevTab[FftNum] == NULL");
            BitRevTab[FftNum][0] = 0;
#else
            WorkData = (sah_complex *)malloc_a(FftLen * sizeof(sah_complex),MEM_ALIGN);
            sah_complex *scratch=(sah_complex *)malloc_a(FftLen*sizeof(sah_complex),MEM_ALIGN);
            if ((WorkData == NULL) || (scratch==NULL)) {
                SETIERROR(MALLOC_FAILED, "WorkData == NULL || scratch == NULL");
            }
            
            // TODO: Deallocate these at the end of the function
            analysis_plans[FftNum] = fftwf_plan_dft_1d(FftLen, scratch, WorkData, FFTW_BACKWARD,
#ifdef ANDROID
                                                       FFTW_ESTIMATE
#else
                                                       FFTW_MEASURE
#endif
                                                       |FFTW_PRESERVE_INPUT);
#endif
            FftNum++;
#if defined(USE_FFTWF) && !defined(USE_FFTS)
            free_a(scratch);
            free_a(WorkData);
#endif /* USE_FFTWF */

        }
        FftLen*=2;
        bitfield>>=1;
    }
#if defined(HAVE_PFFFT)
    if (usePFFFT2)  // use PFFFT
        pffft_work = (float*)malloc_a(2*sizeof(float)*maxFftLen, MEM_ALIGN);
#endif

#ifdef USE_FFTWF
    {
        float *out= (float *)malloc_a(ac_fft_len*sizeof(float),MEM_ALIGN);
        float *scratch=(float *)malloc_a(ac_fft_len*sizeof(float),MEM_ALIGN);
        if ((out == NULL) || (scratch==NULL)) {
            SETIERROR(MALLOC_FAILED, "out == NULL || scratch == NULL");
        }
        autocorr_plan=fftwf_plan_r2r_1d(ac_fft_len, scratch, out, FFTW_REDFT10,
#ifdef ANDROID
                                        FFTW_ESTIMATE
#else
                                        FFTW_MEASURE
#endif
                                        |FFTW_PRESERVE_INPUT);
        free_a(scratch);
	free_a(out);
    }
    
    wisdom=boinc_fopen("wisdom.sah","w");
    if (wisdom) {
        char *wiz=fftwf_export_wisdom_to_string();
        if (wiz) {
            fwrite(wiz,strlen(wiz),1,wisdom);
        }
        fclose(wisdom);
    }
#endif

    if (!state.icfft) {
#ifdef CUSTOM_STRING
        fprintf(stderr,"%s\n", CUSTOM_STRING);
#else
	fprintf(stderr,"%s\n", SAH_PACKAGE_STRING);
#endif
#ifdef AVX_EMU
    fprintf(stderr,"Emulating AVX\n");
#endif
	fprintf(stderr,"%s\n\n", BOINC_PACKAGE_STRING);
        fprintf(stderr,"Work Unit Info:\n");
        fprintf(stderr,"...............\n");
        fprintf(stderr,"WU true angle range is :  %f\n", swi.angle_range);
    } else fprintf(stderr,"Restarted at %.2f percent.\n", progress*100);
    fflush(stderr);

    swi.num_fft_lengths=FftNum;
    // gernerate table of chirp/fft pairs (we may read table from file if testing)
    if (cfft_file != NULL)
        num_cfft = ReadCFftFile(&ChirpFftPairs, &MinChirpStep);
    else
        num_cfft = GenChirpFftPairs(&ChirpFftPairs, &MinChirpStep);
    if (num_cfft == MALLOC_FAILED) {
        SETIERROR(MALLOC_FAILED, "num_cfft == MALLOC_FAILED");
    }

    // Get together various values that we'll need to analyse power over time
    ComputePoTInfo(num_cfft, NumDataPoints);

    // Initialize TrigArrays for testing if we have the memory....
    // If we can't tell how much memory we have, assume we have plenty.
    if ((app_init_data.host_info.m_nbytes == 0)  ||
            (app_init_data.host_info.m_nbytes >= (double)(64*1024*1024))) {
        InitTrigArray (NumDataPoints, MinChirpStep,
                       TESTCHIRPIND,
                       swi.subband_sample_rate);
    }

    boinc_install_signal_handlers();
#ifdef BOINC_APP_GRAPHICS
    if (sah_graphics) strcpy(sah_graphics->status, "Choosing optimal functions");
#endif
    // Choose the best analysis functions.
    ChooseFunctions(&BaseLineSmooth,
                    &GetPowerSpectrum,
                    &ChirpData,
                    &Transpose,
                    ChirpFftPairs,
                    num_cfft,
                    swi.nsamples,
                    state.icfft == 0);

    if ((app_init_data.host_info.m_nbytes == 0)  ||
            (app_init_data.host_info.m_nbytes >= (double)(64*1024*1024))) {
        FreeTrigArray();
        // If we're using TrigArrays, reallocate & reinit
        if ((ChirpData == v_ChirpData) || (ChirpData == fpu_opt_ChirpData)) {
            InitTrigArray (NumDataPoints, MinChirpStep,
                           ChirpFftPairs[state.icfft].ChirpRateInd,
                           swi.subband_sample_rate);
        }
    }

#ifdef USE_IPP
    if (MaxBufSize) {
        FftBuf = (Ipp8u*) malloc_a (MaxBufSize, MEM_ALIGN);
        if (FftBuf == NULL) SETIERROR (MALLOC_FAILED, "FftBuf == NULL");
    }
#elif !defined(USE_FFTS) && !defined(USE_FFTWF)
    for (FftNum = 0; FftNum < swi.num_fft_lengths; FftNum++) {
        CoeffTab[FftNum] = (float*) calloc_a(swi.analysis_fft_lengths[FftNum]/2, sizeof(float), MEM_ALIGN);
        if (CoeffTab[FftNum] == NULL) SETIERROR(MALLOC_FAILED, "CoeffTab[FftNum] == NULL");
    }
#endif

    // Allocate WorkData array the size of the biggest FFT we'll do
    // TODO: Deallocate this at the end of the function
    WorkData = (sah_complex *)malloc_a(FftLen/2 * sizeof(sah_complex),MEM_ALIGN);
    if (WorkData == NULL) {
        SETIERROR(MALLOC_FAILED, "WorkData == NULL");
    }

    // Smooth Baseline

#ifdef DO_SMOOTH
#ifdef BOINC_APP_GRAPHICS
    if (sah_graphics) strcpy(sah_graphics->status, "Doing Baseline Smoothing");
#endif
    retval = BaseLineSmooth(
                 DataIn, NumDataPoints, swi.analysis_cfg.bsmooth_boxcar_length,
                 swi.analysis_cfg.bsmooth_chunk_size
             );
    if (retval) SETIERROR(retval,"from BaseLineSmooth");
#endif


    // used to calculate percent done
    //ProgressUnitSize = GetProgressUnitSize(NumDataPoints, num_cfft, swi);
    ProgressUnitSize = GetProgressUnitSize(NumDataPoints, num_cfft, ac_fft_len);
    //#define DUMP_CHIRP
#ifdef DUMP_CHIRP
    // dump chirp/fft pairs and exit.
    fprintf(stderr, "size  = %d MinChirpStep = %f\n", num_cfft, MinChirpStep);
    for (icfft = 0; icfft < num_cfft; icfft++) {
        fprintf(stderr,"%6d %15.11f %6d %6d %d %d\n",
                icfft,
                ChirpFftPairs[icfft].ChirpRate,
                ChirpFftPairs[icfft].ChirpRateInd,
                ChirpFftPairs[icfft].FftLen,
                ChirpFftPairs[icfft].GaussFit,
                ChirpFftPairs[icfft].PulseFind
               );
    }
    fflush(stderr);
    exit(0);
#endif


    boinc_wu_cpu_time(cputime0);
    reset_units();
    double chirp_units=0;

    // Loop through chirp/fft pairs - this is the top level analysis loop.
    double last_ptime=0;
    int rollovers=0;
    double clock_max=0;
    for (icfft = state.icfft; icfft < num_cfft; icfft++) {
        fftlen    = ChirpFftPairs[icfft].FftLen;
        chirprate = ChirpFftPairs[icfft].ChirpRate;
        chirprateind = ChirpFftPairs[icfft].ChirpRateInd;
        // boinc_worker_timer();
#ifdef DEBUG
        double ptime=static_cast<double>((unsigned)clock())/CLOCKS_PER_SEC+
                     clock_max*rollovers;
        clock_max=std::max(last_ptime-ptime,clock_max);
        if (ptime<last_ptime) {
            rollovers++;
            ptime=static_cast<double>((unsigned)clock())/CLOCKS_PER_SEC+
                  clock_max*rollovers;
        }
        last_ptime=ptime;

        fprintf(stderr,"%f %f %f %f %f %f %f %f %f\n",
                ptime,
                progress,
                ((double)icfft)/num_cfft,
                analysis_state.FLOP_counter,
                triplet_units,
                pulse_units,
                spike_units,
                gauss_units,
                chirp_units
               );
        fflush(stderr);

        double cputime=0;
        boinc_wu_cpu_time(cputime);
        cputime-=cputime0;
#endif

        remaining=1.0-(double)icfft/num_cfft;

        if (chirprateind != last_chirp_ind) {
#ifdef BOINC_APP_GRAPHICS
            if (sah_graphics) strcpy(sah_graphics->status, "Chirping data");
#endif

            retval = ChirpData(
                         DataIn,
                         ChirpedData,
                         chirprateind,
                         chirprate,
                         NumDataPoints,
                         swi.subband_sample_rate
                     );

            if (retval) SETIERROR(retval, "from ChirpData()");

            progress += (double)(ProgressUnitSize * ChirpProgressUnits());
            chirp_units+=(double)(ProgressUnitSize * ChirpProgressUnits());
            progress = std::min(progress,1.0);
        }

        //    last_chirp = chirprate;
        last_chirp_ind = chirprateind;

        // Process this FFT length.
        // For a given FFT length (at a given chirp), we construct
        // PowerSpectrum[] which is a "waterfall" array of power spectra,
        // each fftlen long on the frequency axis and sample time long
        // on the time axis.
        // As we go along, we check each spectra for spikes.

        state.icfft = icfft;     // update analysis state

        // Find index into FFT length table for the current
        // FFT length.  This will be the same index needed
        // for ooura's coeffecient and bit reverse tables.
        for (FftNum = 0; FftNum < swi.num_fft_lengths; FftNum++) {
            if (swi.analysis_fft_lengths[FftNum] == fftlen) {
                break;
            }
        }

#ifdef BOINC_APP_GRAPHICS
        if (sah_graphics) {
            sah_graphics->fft_info.chirp_rate = chirprate;
            sah_graphics->fft_info.fft_len = fftlen;
            strcpy(sah_graphics->status, "Computing Fast Fourier Transform");
        }
#endif

        // If PoT freq bin is non-negative, we are into PoT analysis
        // for this cfft pair and should not re-output an "ogh" line.
        if (state.PoT_freq_bin == -1) {
            retval = result_group_start();
            if (retval) SETIERROR(retval,"from result_group_start");
        }

        // Number of FFTs for this length
        NumFfts   = NumDataPoints / fftlen;

#ifdef BOINC_APP_GRAPHICS
        if (sah_graphics) {
            rarray.init_data(fftlen, NumFfts);
        }
#endif
        for (ifft = 0; ifft < NumFfts; ifft++) {
            // boinc_worker_timer();
            CurrentSub = fftlen * ifft;
#if !defined(USE_FFTS) && !defined(USE_FFTWF) && !defined(USE_IPP)
            // FFTW and IPP now use out of place transforms.
            memcpy(
                WorkData,
                &ChirpedData[CurrentSub],
                (int)(fftlen * sizeof(sah_complex))
            );
#endif

            state.FLOP_counter+=5*(double)fftlen*log((double)fftlen)/log(2.0);
#ifdef USE_IPP
            ippsFFTInv_CToC_32fc((Ipp32fc*)ChirpedData[CurrentSub],
                                 (Ipp32fc*)WorkData,
                                 FftSpec[FftNum], FftBuf);
#elif defined(USE_FFTS)
#ifdef USE_FFTS_VFP_P
            if (!useFFTSNEON)
                vfp_ffts_execute(analysis_plans_vfp[FftNum], &ChirpedData[CurrentSub], WorkData);
            else
#endif
            ffts_execute(analysis_plans[FftNum], &ChirpedData[CurrentSub], WorkData);
#elif defined(USE_FFTWF)
            fftwf_execute_dft(analysis_plans[FftNum], &ChirpedData[CurrentSub], WorkData);
#else
            // replace time with freq - ooura FFT
            cdft(fftlen*2, 1, WorkData, BitRevTab[FftNum], CoeffTab[FftNum]);
#endif

            // replace freq with power
            state.FLOP_counter+=(double)fftlen;
            GetPowerSpectrum( WorkData,
                              &PowerSpectrum[CurrentSub],
                              fftlen
                            );

	    if (fftlen==(long)ac_fft_len) {
	      state.FLOP_counter+=((double)fftlen)*5*log((double)fftlen)/log(2.0)+2*fftlen;
#if defined(USE_FFTWF)
              fftwf_execute_r2r(autocorr_plan,&PowerSpectrum[CurrentSub],AutoCorrelation);
#else
#error Analysis needs to be modified for your FFT choice.
#endif
            }
	        
            // any ETIs ?!
            // If PoT freq bin is non-negative, we are into PoT analysis
            // for this cfft pair and need not redo spike and autocorr finding.
            if (state.PoT_freq_bin == -1) {
                state.FLOP_counter+=(double)fftlen;
                retval = FindSpikes(
                             &PowerSpectrum[CurrentSub],
                             fftlen,
                             ifft,
                             swi
                         );
                if (retval) SETIERROR(retval,"from FindSpikes");
                
                if (fftlen==ac_fft_len) {
                  retval = FindAutoCorrelation(
                               AutoCorrelation,
                               fftlen,
                               ifft,
                               swi
                           );
                  if (retval) SETIERROR(retval,"from FindAutoCorrelation");
                  progress += 2.0*SpikeProgressUnits(fftlen)*ProgressUnitSize/NumFfts;
                } else {
                  progress += SpikeProgressUnits(fftlen)*ProgressUnitSize/NumFfts;
                }
            }

            //progress = ((float)icfft)/num_cfft + ((float)ifft)/(NumFfts*num_cfft);
            progress = std::min(progress,1.0);
#ifdef BOINC_APP_GRAPHICS
            if (!nographics()) {
                rarray.add_source_row(PowerSpectrum+fftlen*ifft);
                sah_graphics->local_progress = (((float)ifft+1)/NumFfts);
            }
#endif
            remaining=1.0-(double)(icfft+1)/num_cfft;
            fraction_done(progress,remaining);
            // jeffc
            //fprintf(stderr, "S fft len %d  progress = %12.10f\n", fftlen, progress);
        } // loop through chirped data array

#ifdef BOINC_APP_GRAPHICS
        if (!nographics()) {
            memcpy(&sah_shmem->rarray_data, &rarray, sizeof(REDUCED_ARRAY_DATA));
        }
#endif
        fraction_done(progress,remaining);
        // jeffc
        //fprintf(stderr, "Sdone fft len %d  progress = %12.10f\n", fftlen, progress);

        // transpose PoT matrix to make memory accesses nicer
        need_transpose = ChirpFftPairs[icfft].GaussFit || ChirpFftPairs[icfft].PulseFind;
        if ( !need_transpose ) {
            int tmpPulsePoTLen, tmpOverlap;
            GetPulsePoTLen( NumFfts, &tmpPulsePoTLen, &tmpOverlap );
            if ( ! ( tmpPulsePoTLen > PoTInfo.TripletMax || tmpPulsePoTLen < PoTInfo.TripletMin ) )
                need_transpose = true;
        }

        if (need_transpose && use_transposed_pot) {
            Transpose(fftlen, NumFfts, (float *) PowerSpectrum, (float *)tPowerSpectrum);
        }
        //
        // Analyze Power over Time.  May return quickly if this FFT
        // length and/or this WUs slew rate places the data block
        // outside PoT analysis limits.
        // Counting flops is done inside analyze_pot
        retval = analyze_pot(tPowerSpectrum, NumDataPoints, ChirpFftPairs[icfft]);
        if (retval) SETIERROR(retval,"from analyze_pot");

#ifdef BOINC_APP_GRAPHICS
        // switch the display back to "best of" signals
        //
        if (!nographics()) {
            sah_graphics->gi.copy(best_gauss, true);
            sah_graphics->pi.copy(best_pulse, true);
            sah_graphics->ti.copy(best_triplet, true);
        }
#endif
        // Force progress to 100% before calling result_group_end() to store
        //  100% in state file so it will survive exit & relaunch
        if (icfft == (num_cfft-1)) {
            progress = 1;
            remaining = 0;
            fraction_done(progress,remaining);
        }
        retval = checkpoint();
        if (retval) SETIERROR(retval,"from checkpoint() in seti_analyse()");

    } // loop over chirp/fftlen paris

    // Return the "best of" signals.  This may include duplicates of
    // already reported interesting signals.
    if (best_spike->s.fft_len) {
        retval = outfile.printf("%s", best_spike->s.print_xml(0,0,1,"best_spike").c_str());
        if (retval < 0) {
            SETIERROR(WRITE_FAILED,"from outfile.printf (best spike) in seti_analyze()");
        }
    }

    if (best_autocorr->a.fft_len) {
        retval = outfile.printf("%s", best_autocorr->a.print_xml(0,0,1,"best_autocorr").c_str());
        if (retval < 0) {
            SETIERROR(WRITE_FAILED,"from outfile.printf (best autocorr) in seti_analyze()");
        }
    }

    if (best_gauss->g.fft_len || best_gauss->score) {
        retval = outfile.printf("%s", best_gauss->g.print_xml(0,0,1,"best_gaussian").c_str());
        if (retval < 0) {
            SETIERROR(WRITE_FAILED,"from outfile.printf (best gaussian) in seti_analyze()");
        }
    }
    if (best_pulse->p.fft_len) {
        retval = outfile.printf("%s", best_pulse->p.print_xml(0,0,1,"best_pulse").c_str());
        if (retval < 0) {
            SETIERROR(WRITE_FAILED,"from outfile.printf (best pulse) in seti_analyze()");
        }
    }
    if (best_triplet->t.fft_len) {
        retval = outfile.printf("%s", best_triplet->t.print_xml(0,0,1,"best_triplet").c_str());
        if (retval < 0) {
            SETIERROR(WRITE_FAILED,"from outfile.printf (best triplet) in seti_analyze()");
        }
    }

#ifdef BOINC_APP_GRAPHICS
    if (sah_graphics) strcpy(sah_graphics->status, "Work unit done");
#endif
    final_report(); // flop and signal counts to stderr
    retval = checkpoint();  // try a final checkpoint

    if (PowerSpectrum) free_a(PowerSpectrum);
    if (use_transposed_pot) free_a(tPowerSpectrum);

#ifdef USE_IPP
    for (FftNum = 0; FftNum < swi.num_fft_lengths; FftNum++) {
        if (FftSpec[FftNum]) ippsFFTFree_C_32fc (FftSpec[FftNum]);
    }
    if (FftBuf) free_a(FftBuf);
    FftBuf = NULL;
#elif !defined(USE_FFTS) && !defined(USE_FFTWF)
    for (FftNum = 0; FftNum < swi.num_fft_lengths; FftNum++) {
        if (BitRevTab[FftNum]) free_a(BitRevTab[FftNum]);
    }
    for (FftNum = 0; FftNum < swi.num_fft_lengths; FftNum++) {
        if (CoeffTab[FftNum]) free_a(CoeffTab[FftNum]);
    }
#endif

#ifdef HAVE_PFFFT
    if (pffft_work!=NULL)
        free_a(pffft_work);
#endif
    if (WorkData) free_a(WorkData);
    WorkData = NULL;

    if (ChirpFftPairs) free(ChirpFftPairs);
    if ((app_init_data.host_info.m_nbytes == 0)  ||
            (app_init_data.host_info.m_nbytes >= (double)(64*1024*1024))) {
        FreeTrigArray();
    }

    // jeffc
    //retval = outfile.flush();
    xml_indent(-2);
    outfile.printf("</result>");
    outfile.close();
    //if (retval) SETIERROR(WRITE_FAILED,"from outfile.fflush in seti_analyze()");

    return retval;
}


int v_BaseLineSmooth(
    sah_complex* DataIn,
    int NumDataPoints,
    int BoxCarLength,
    int NumPointsInChunk
) {

    // We use a sliding boxcar method for baseline smoothing.  Input data
    // is the time domain.  It is transformed (using a separate array)
    // into the frequency domain.  After baseline smoothing is done
    // in the frequency domain, it is transformed back into the time domain.

    int h, i, j, k;
    sah_complex* DataInChunk;
    static sah_complex *DataOutChunk=0;
    static float* PowerSpectrum=0;
    float Total, LocalMean, ScaleFactor,recipNumPointsInChunk=1.0/NumPointsInChunk;
    int NumTimeChunks, TimeChunk, Endpoint;
    static int OldNumPointsInChunk = 0;
#ifdef HAVE_PFFFT
    static PFFFT_Setup* pffft_x_transform = NULL;
#endif
#ifdef USE_IPP
    static IppsFFTSpec_C_32fc* FftSpec = NULL;
#elif defined(USE_FFTS)
    static ffts_plan_t* backward_transform = NULL;
    static ffts_plan_t* forward_transform = NULL;
    static vfp_ffts_plan_t* backward_transform_vfp = NULL;
    static vfp_ffts_plan_t* forward_transform_vfp = NULL;
#elif defined(USE_FFTWF)
    static fftwf_plan backward_transform, forward_transform;
#endif /* USE_FFTWF */

#if !defined(USE_FFTS) && !defined(USE_FFTWF)
    static int * BitRevTab = NULL;
    static float * CoeffTab = NULL;
#endif /* USE_FFTWF */

    NumTimeChunks = (int)(NumDataPoints * recipNumPointsInChunk);


    // If we keep doing transforms that are the same length, don't reinitialize plans
    if (NumPointsInChunk != OldNumPointsInChunk) {
        if (OldNumPointsInChunk != 0) {

#ifdef USE_IPP
            if (FftSpec) ippsFFTFree_C_32fc (FftSpec);
#elif defined(USE_FFTS)
#  ifdef HAVE_PFFFT
            if (usePFFFT2)
                pffft_destroy_setup(pffft_x_transform);
            else
            {
#  endif

#ifdef USE_FFTS_VFP_P
                if (useFFTSNEON)
                {
#endif
                    if (backward_transform) ffts_free(backward_transform);
                    if (forward_transform) ffts_free(forward_transform);
#ifdef USE_FFTS_VFP_P
                }
                else
                {
                    if (backward_transform_vfp) vfp_ffts_free(backward_transform_vfp);
                    if (forward_transform_vfp) vfp_ffts_free(forward_transform_vfp);
                }
#endif
#  ifdef HAVE_PFFFT
            }
#  endif
#elif defined(USE_FFTWF)
            if (backward_transform) free_a(backward_transform);
            if (forward_transform) free_a(forward_transform);
#else
            if (BitRevTab) free_a(BitRevTab);
            if (CoeffTab) free_a(CoeffTab);
#endif
            if (PowerSpectrum) free_a(PowerSpectrum);
            if (DataOutChunk) free_a(DataOutChunk);
        }
        PowerSpectrum = (float*) calloc_a(NumPointsInChunk, sizeof(float), MEM_ALIGN);
        if (PowerSpectrum == NULL) {
            printf("Could not allocate Power Spectrum array in v_BaseLineSmooth()\n");
            exit(1);
        }
        // Do the transforms in the DataOutChunk, since DataInChunk won't
        // necessarily be aligned correctly (and we won't get SIMD)
        // TODO: automatically make DataInChunk aligned correctly so we
        //       don't need the memcpy. This may already be done if
        //       NumPointsInChunk*sizeof(sah_complex) is a multiple of MEM_ALIGN
        DataOutChunk = (sah_complex *)malloc_a(NumPointsInChunk * sizeof(sah_complex),MEM_ALIGN);
        OldNumPointsInChunk = NumPointsInChunk;

#ifdef USE_IPP
        int order = 0;
        for (int tmp = NumPointsInChunk; !(tmp & 1); order++) tmp >>= 1;
        ippsFFTInitAlloc_C_32fc (&FftSpec, order, IPP_FFT_NODIV_BY_ANY,
                                 ippAlgHintAccurate);
#elif defined(USE_FFTS)
#  ifdef HAVE_PFFFT
        if (usePFFFT2)
            pffft_x_transform = pffft_new_setup(NumPointsInChunk, PFFFT_COMPLEX);
        else
        {
#  endif
#ifdef USE_FFTS_VFP_P
            if (useFFTSNEON)
            {
#endif
                backward_transform = ffts_init_1d(NumPointsInChunk, 1);
                forward_transform = ffts_init_1d(NumPointsInChunk, -1);
#ifdef USE_FFTS_VFP_P
            }
            else
            {
                backward_transform_vfp = vfp_ffts_init_1d(NumPointsInChunk, 1);
                forward_transform_vfp = vfp_ffts_init_1d(NumPointsInChunk, -1);
            }
#endif
#  ifdef HAVE_PFFFT
        }
#  endif
#elif defined(USE_FFTWF)
        sah_complex *scratch = (sah_complex *)malloc_a(NumPointsInChunk * sizeof(sah_complex),MEM_ALIGN);

        backward_transform = fftwf_plan_dft_1d(NumPointsInChunk, scratch, DataOutChunk, FFTW_BACKWARD,
#ifdef ANDROID
                                               FFTW_ESTIMATE
#else
                                               FFTW_MEASURE
#endif
                                               |FFTW_PRESERVE_INPUT);
        forward_transform = fftwf_plan_dft_1d(NumPointsInChunk, DataOutChunk, DataOutChunk, FFTW_FORWARD,
#ifdef ANDROID
                                               FFTW_ESTIMATE
#else
                                               FFTW_MEASURE
#endif            
            );
        free_a(scratch);
#else
        BitRevTab = (int*) calloc_a(3+(int)sqrt((float)NumPointsInChunk/2), sizeof(int), MEM_ALIGN);
        if (BitRevTab == NULL) return MALLOC_FAILED;

        CoeffTab = (float*) calloc_a(NumPointsInChunk/2, sizeof(float), MEM_ALIGN);
        if (CoeffTab == NULL) return MALLOC_FAILED;
        // flag to tell cdft() to init it's work areas
        // already done since we used calloc_a();
        // BitRevTab[0] = 0;
#endif

    }

    for (TimeChunk = 0; TimeChunk < NumTimeChunks; TimeChunk++) {
#ifdef BOINC_APP_GRAPHICS
        if (sah_graphics) sah_graphics->local_progress = (((float)TimeChunk)/NumTimeChunks);
#endif

        DataInChunk = &(DataIn[TimeChunk*NumPointsInChunk]);
#if !defined(USE_FFTS) && !defined(USE_FFTWF)
        memcpy( DataOutChunk, DataInChunk, (int)(NumPointsInChunk*sizeof(sah_complex)) );
#endif

        // transform to freq
#ifdef USE_IPP
        ippsFFTInv_CToC_32fc ((Ipp32fc*)DataOutChunk, (Ipp32fc*)DataOutChunk,
                              FftSpec, NULL);
#elif defined(USE_FFTS)
#  ifdef HAVE_PFFFT
        if (usePFFFT2)
            pffft_transform(pffft_x_transform, (const float*)DataInChunk,
                            (float*)DataOutChunk, pffft_work, PFFFT_BACKWARD);
        else
#  endif
        {
#  ifdef USE_FFTS_VFP_P
            if (useFFTSNEON)
#  endif
                ffts_execute(backward_transform,DataInChunk,DataOutChunk);
#  ifdef USE_FFTS_VFP_P
            else
                vfp_ffts_execute(backward_transform_vfp,DataInChunk,DataOutChunk);
#  endif
        }
#elif !defined(USE_FFTWF)
        cdft(NumPointsInChunk*2, 1, DataOutChunk, BitRevTab, CoeffTab);
#else
        fftwf_execute_dft(backward_transform,DataInChunk,DataOutChunk);
#endif

        GetPowerSpectrum(
            DataOutChunk, PowerSpectrum, NumPointsInChunk
        );

        // Begin: normalize in freq. domain via sliding boxcar

        Endpoint = NumPointsInChunk / 2;

        i = Endpoint;         // start i at lowest negative freq;
        // this is low point in first boxcar
        j = i + BoxCarLength / 2;        // start j midpoint in first boxcar
        k = i + BoxCarLength;            // start k at end of first boxcar;
        // thus a boxcar is i----j----k
        Total = 0;
        for (h = i; h < k; h++) {            // Get mean for first boxcar
            Total += PowerSpectrum[h];
        }
        LocalMean = Total / BoxCarLength;
        ScaleFactor = (float)(1.0/sqrt(LocalMean * 0.5));

        // normalize 1st half of 1st boxcar
        for (h = i; h < j; h++) {
            DataOutChunk[h][0] *= ScaleFactor;
            DataOutChunk[h][1] *= ScaleFactor;
        }

        for (; k != Endpoint; i++, j++, k++) {  // sliding boxcar
            if (k == NumPointsInChunk) {    // take care of wrapping
                k = 0;
            }
            if (j == NumPointsInChunk) {
                j = 0;
            }
            if (i == NumPointsInChunk) {
                i = 0;
            }

            LocalMean = LocalMean
                        - PowerSpectrum[i] / BoxCarLength
                        + PowerSpectrum[k] / BoxCarLength;
            ScaleFactor = (float)(1.0/sqrt(LocalMean * 0.5));

            DataOutChunk[j][0] *= ScaleFactor;
            DataOutChunk[j][1] *= ScaleFactor;

        } // End sliding boxcar

        // normalize final half of final boxcar
        for (h = j; h < k; h++) {
            DataOutChunk[h][0] *= ScaleFactor;
            DataOutChunk[h][1] *= ScaleFactor;
        }

        // End: normalize in freq. domain via sliding boxcar

        // transform back to time
#ifdef USE_IPP
        ippsFFTFwd_CToC_32fc((Ipp32fc*)DataOutChunk, (Ipp32fc*)DataOutChunk,
                             FftSpec, NULL);
#elif defined(USE_FFTS)
#  ifdef HAVE_PFFFT
        if (usePFFFT2)
            pffft_transform(pffft_x_transform, (const float*)DataOutChunk,
                            (float*)DataInChunk, pffft_work, PFFFT_FORWARD);
        else
#  endif
        {
#  ifdef USE_FFTS_VFP_P
            if (useFFTSNEON)
#  endif
                ffts_execute(forward_transform, DataOutChunk, DataInChunk);
#  ifdef USE_FFTS_VFP_P
            else
                vfp_ffts_execute(forward_transform_vfp, DataOutChunk, DataInChunk);
#  endif
        }
#elif !defined(USE_FFTWF)
        cdft(NumPointsInChunk*2, -1, DataOutChunk, BitRevTab, CoeffTab);
#else
        fftwf_execute(forward_transform);
#endif
        analysis_state.FLOP_counter+=10.0*NumPointsInChunk*log((double)NumPointsInChunk)/log(2.0)+10.0*NumPointsInChunk;
        // return powers to normal
        for (i = 0; i < NumPointsInChunk; i++) {
#if defined(USE_FFTS)
            DataInChunk[i][0] = DataInChunk[i][0]*recipNumPointsInChunk;
            DataInChunk[i][1] = DataInChunk[i][1]*recipNumPointsInChunk;
#else
            DataInChunk[i][0] = DataOutChunk[i][0]*recipNumPointsInChunk;
            DataInChunk[i][1] = DataOutChunk[i][1]*recipNumPointsInChunk;
#endif
        }
    }

    return 0;
}


int v_GetPowerSpectrum(
    sah_complex* FreqData,
    float* PowerSpectrum,
    int NumDataPoints
) {
    int i;

#ifdef __INTEL_COMPILER
#pragma message ("---using ICC---")
#pragma vector aligned
    __assume_aligned (FreqData, MEM_ALIGN);
#endif
    analysis_state.FLOP_counter+=3.0*NumDataPoints;
    for (i = 0; i < NumDataPoints; i++) {
        PowerSpectrum[i] = FreqData[i][0] * FreqData[i][0]
                           + FreqData[i][1] * FreqData[i][1];
    }
    return 0;
}

// chirp_rate is in Hz per second

#ifndef USE_INTEL_OPT_CODE
int v_ChirpData(
    sah_complex* cx_DataArray,
    sah_complex* cx_ChirpDataArray,
    int chirp_rate_ind,
    double chirp_rate,
    int  ul_NumDataPoints,
    double sample_rate
) {
    int i;
    double recip_sample_rate=1.0/sample_rate;

#ifdef DEBUG
    fprintf(stderr, "icfft = %6d  crate index = %6d\n", icfft, chirp_rate_ind);
    fflush(stderr);
#endif

    if (chirp_rate_ind == 0) {
        memcpy(cx_ChirpDataArray,
               cx_DataArray,
               (int)ul_NumDataPoints * sizeof(sah_complex)
              );    // NOTE INT CAST
    } else {

        // what we do depends on how much memory we have...
        // If we have more than 64MB, we'll cache the chirp table.  If not
        // we'll calculate it each time.
        bool CacheChirpCalc=((app_init_data.host_info.m_nbytes == 0)  ||
                             (app_init_data.host_info.m_nbytes >= (double)(64*1024*1024)));

        // calculate trigonometric array
        // this function returns w/o doing nothing when sign of chirp_rate_ind
        // reverses.  so we have to take care of it.

        if (CacheChirpCalc) CalcTrigArray(ul_NumDataPoints, chirp_rate_ind);

        for (i = 0; i < ul_NumDataPoints; i++) {
            float c, d, real, imag;

            if (CacheChirpCalc) {
                c = CurrentTrig[i].Cos;
                d = (chirp_rate_ind >0)? CurrentTrig[i].Sin : -CurrentTrig[i].Sin;
            } else {
                double dd,cc;
                double time=static_cast<double>(i)*recip_sample_rate;
                // since ang is getting moded by 2pi, we calculate "ang mod 2pi"
                // before the call to sincos() inorder to reduce roundoff error.
                // (Bug submitted by Tetsuji "Maverick" Rai)
                double ang  = 0.5*chirp_rate*time*time;
                ang -= floor(ang);
                ang *= M_PI*2;
                sincos(ang,&dd,&cc);
                c=cc;
                d=dd;
            }

            // Sometimes chirping is done in place.
            // We don't want to overwrite data prematurely.
            real = cx_DataArray[i][0] * c - cx_DataArray[i][1] * d;
            imag = cx_DataArray[i][0] * d + cx_DataArray[i][1] * c;
            cx_ChirpDataArray[i][0] = real;
            cx_ChirpDataArray[i][1] = imag;
        }
        analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;
    }
    return 0;
}
#endif // USE_INTEL_OPT_CODE


// Trigonometric arrays functions
// These functions makes trigonometric arrays very quickly and
// will speed up v_ChirpData()
//   by Tetsuji "Maverick" Rai

// initialize TrigStep and CurrentTrig

void InitTrigArray(int len, double ChirpStep, int InitChirpRateInd,
                   double SampleRate) {
    int i;
    double ang, Coef;

    TrigStep = (SinCosArray*) malloc_a (len * sizeof(SinCosArray), MEM_ALIGN);
    if (TrigStep == NULL) SETIERROR(MALLOC_FAILED, "TrigStep == NULL");

    CurrentTrig = (SinCosArray*) malloc_a (len * sizeof(SinCosArray), MEM_ALIGN);
    if (CurrentTrig == NULL) SETIERROR(MALLOC_FAILED, "CurrentTrig == NULL");

    // Make ChirpStep array

    Coef = ChirpStep / (SampleRate*SampleRate);

    for (i = 0; i < len; i++) {
        // since ang is getting cast to a float, we calculate "ang mod 2pi"
        // before the call to sincosf() inorder to reduce roundoff error.
        // (Bug submitted by Tetsuji "Maverick" Rai)
        // addition: now it's used as double float, but this bug fix
        // is still preferable
        ang = 0.5*(double)i*(double)i*Coef;
        ang -= floor(ang);
        ang *= 2*M_PI;
        sincos(ang, &TrigStep[i].Sin, &TrigStep[i].Cos);
    }

    // Set initial trigonometric array

    if ((CurrentChirpRateInd = abs(InitChirpRateInd)) != 0) {
        Coef = CurrentChirpRateInd*ChirpStep / (SampleRate*SampleRate);

        for (i = 0; i < len; i++) {
            ang = 0.5*(double)i*(double)i*Coef;
            ang -= floor(ang);
            ang *= 2*M_PI;
            sincos(ang, &CurrentTrig[i].Sin, &CurrentTrig[i].Cos);
        }
    } else {
        // if it starts from the beginning, it's quite simple
        for (i = 0; i < len; i++) {
            CurrentTrig[i].Sin = 0.;
            CurrentTrig[i].Cos = 1.0;
        }
    }
}



// calculate next CurrentTrig

void CalcTrigArray (int len, int ChirpRateInd) {
    double  SinX, CosX, SinS, CosS;
    int TempCRateInd = abs(ChirpRateInd);
    int i, j;
    // skip automatically when TempCRateInd == CurrentChirpRateInd
    //   (it happens when sign of chirprate reverses)
    // Otherwise
    //     sin(x+step) = sin(x)*cos(step) + cos(x)*sin(step)
    //     cos(x+step) = cos(x)*cos(step) - sin(x)*sin(step)

    // In most cases index increases by 1, and in the later phase of a WU
    // by 4 at most in reference WU, but it's 2-3 times faster than
    // sincos() function on P4 with gcc or icc.  When index increases by 1,
    // this is about 10 times as fast as sincos()

    // JWS: Modified so when index increases by more than one, the two 16 MiB
    // arrays are only loaded once. The index can increase by as much as 16.
    //
#ifdef DEBUG
    fprintf(stderr, " New ind = %6d (abs(%6d))  Previous = %6d\n",
            ChirpRateInd, TempCRateInd, CurrentChirpRateInd);
    fflush(stderr);
#endif
  
    switch ( TempCRateInd - CurrentChirpRateInd ) {
        case 0:
            return;        // replaces "automatic skip"
        case 1:
            for ( j = 0; j < len; j++ ) {
                SinX = CurrentTrig[j].Sin;
                CosX = CurrentTrig[j].Cos;
                SinS = TrigStep[j].Sin;
                CosS = TrigStep[j].Cos;

                CurrentTrig[j].Sin = SinX * CosS + CosX * SinS;
                CurrentTrig[j].Cos = CosX * CosS - SinX * SinS;
            }
            break;
        case 2:        // unroll once to avoid 50/50 inner loop
            for ( j = 0; j < len; j++ ) {
                double  SinTmp, CosTmp;

                SinX = CurrentTrig[j].Sin;
                CosX = CurrentTrig[j].Cos;
                SinS = TrigStep[j].Sin;
                CosS = TrigStep[j].Cos;

                SinTmp = SinX * CosS + CosX * SinS;
                CosTmp = CosX * CosS - SinX * SinS;

                CurrentTrig[j].Sin = SinTmp * CosS + CosTmp * SinS;
                CurrentTrig[j].Cos = CosTmp * CosS - SinTmp * SinS;
            }
            break;
        default:       // 3 or more
            for ( j = 0; j < len; j++ ) {
                for ( i = CurrentChirpRateInd; i < TempCRateInd; i++ ) {
                    SinX = CurrentTrig[j].Sin;
                    CosX = CurrentTrig[j].Cos;
                    SinS = TrigStep[j].Sin;
                    CosS = TrigStep[j].Cos;

                    CurrentTrig[j].Sin = SinX * CosS + CosX * SinS;
                    CurrentTrig[j].Cos = CosX * CosS - SinX * SinS;
                }
            }
            break;
    }
    CurrentChirpRateInd = TempCRateInd;
}


// free TrigStep and CurrentTrig

void FreeTrigArray() {
    if (TrigStep) free_a(TrigStep);
    TrigStep = NULL;

    if (CurrentTrig) free_a(CurrentTrig);
    CurrentTrig = NULL;
}

template <int x>
inline void v_subTranspose(float *in, float *out, int xline, int yline) {
    // Transpose an X by X subsection of a XLINE by YLINE matrix into the
    // appropriate part of a YLINE by XLINE matrix.  "IN" points to the first
    // (lowest address) element of the input submatrix.  "OUT" points to the
    // first (lowest address) element of the output submatrix.
    int i,j;
    float *p;
    register float tmp[x*x];
    for (j=0;j<x;j++) {
        p=in+j*xline;
        for (i=0;i<x;i++) {
            tmp[j+i*x]=*p++;
        }
    }
    for (j=0;j<x;j++) {
        p=out+j*yline;
        for (i=0;i<x;i++) {
            *p++=tmp[i+j*x];
        }
    }
}

int v_Transpose(int x, int y, float *in, float *out) {
    // stupidest possible algorithm
    // assume in and out can't overlap
    int i,j;
    for (j=0;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    return 0;
}

int v_Transpose2(int x, int y, float *in, float *out) {
    // Attempts to improve cache hit ratio by transposing 4 elements at a time.
    int i,j;
    for (j=0;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_subTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    return 0;
}

int v_Transpose4(int x, int y, float *in, float *out) {
    // Attempts to improve cache hit ratio by transposing 16 elements at a time.
    int i,j;
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_subTranspose<4>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_subTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_subTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    return 0;
}

int v_Transpose8(int x, int y, float *in, float *out) {
    // Attempts to improve cache hit ratio by transposing 64 elements at a time.
    int i,j;
    for (j=0;j<y-7;j+=8) {
        for (i=0;i<x-7;i+=8) {
            v_subTranspose<8>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-3;i+=4) {
            v_subTranspose<4>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_subTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (j=0;j<y-3;j+=4) {
        for (i=0;i<x-3;i+=4) {
            v_subTranspose<4>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x-1;i+=2) {
            v_subTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y-1;j+=2) {
        for (i=0;i<x-1;i+=2) {
            v_subTranspose<2>(in+j*x+i,out+y*i+j,x,y);
        }
        for (;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    for (;j<y;j++) {
        for (i=0;i<x;i++) {
            out[i*y+j]=in[j*x+i];
        }
    }
    return 0;
}

