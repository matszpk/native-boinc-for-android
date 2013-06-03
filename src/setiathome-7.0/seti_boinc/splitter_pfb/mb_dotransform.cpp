/*
 * 
 * dotransform.c
 *
 * Functions for division by frequency into work units.
 *
 * $Id: mb_dotransform.cpp,v 1.1.2.3 2007/06/06 15:58:29 korpela Exp $
 *
 */

#include "sah_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <gsl/gsl_sf_trig.h>

#include "setilib.h"
#include "splitparms.h"
#include "splittypes.h"
#include "mb_splitter.h"
#include "fftw3.h"
#include "mb_wufiles.h"

#include <sys/types.h>      /* for open() */
#include <sys/stat.h>       /* for open() */
#include <fcntl.h>          /* for open() */
#include "mb_dotransform.h" /* for new function declarations and macro
                               definitions */

// PFB parameters
int   g_iNTaps;
float g_fWidthFactor;

/* the PFB data structure */
typedef struct 
{
    complex<float>* pcfData;                        // stores the data for one polyphase filtering; PFB_FFT_LEN complex floats 
    fftwf_complex* pfcData;                         // points to the above data 
} PFB_DATA;
PFB_DATA* g_astPFBData;                             // points to g_iNTaps PFB_DATA's


fftwf_plan g_stPlan = {0};                          // FFT plan 
fftwf_complex* g_pfcFFTArray = NULL;                // FFT input/output; PFB_FFT_LEN complex floats

static complex<float>* g_pcfPackedDataOut = NULL;   // packed FFT output, for the benefit of output_samples();
                                                    //  PFB_OUTPUT_STACK_SIZE * PFB_FFT_LEN * 2 floats

float* g_pfPFBCoeff = NULL;                         // filter coefficients; g_iNTaps * PFB_FFT_LEN floats
int g_iIsFirstRun = TRUE;

void gen_coeff(float * coeff, int num_coeff) {

  int i;
  float hanning_window[num_coeff];
  float work_array[num_coeff];

  seti_hanning_window(hanning_window, num_coeff);

  for(i=0; i<num_coeff; i++) {
	coeff[i] = (float(i) / PFB_FFT_LEN) - (float(g_iNTaps) / 2.0);
  }

  for(i=0; i<num_coeff; i++) {
	coeff[i] = gsl_sf_sinc(coeff[i] * g_fWidthFactor) * hanning_window[i];
  }
}

void output_samples(float *data, int i) {
// outputs 8 complex samples per call (8 cplx floats -> 8 cplx chars, or 16 floats -> 16 chars)
  int j,k;
  unsigned short s=0;
  float *p=data;

  for (k=0; k<8; k++) {
      s >>= 2;
      if (*p>0) s |= 0x8000;
      if (*(p+1)>0) s |= 0x4000;
      p+=2;
  }
  // bin_data is defined in mb_wufiles.cpp as a vector<unsigned char>
  bin_data[i].push_back((s>>8) & 0xff);
  bin_data[i].push_back(s & 0xff);
}

void splitter_bits_to_float(unsigned short *raw, float *data, int nsamples) {
    unsigned int i, j;
    unsigned short s;
    static int first_time=1;
    static float lut[65536][16];

    assert(!(nsamples % 8));

    if (first_time) {
      for (i=0;i<65536;i++) {
	s=(unsigned short)i;
	for (j=0;j<8;j++) {
	    lut[i][j*2]=(float)2*(s & 1)-1;
	    s >>= 1;
	    lut[i][j*2+1]=(float)2*(s & 1)-1;
            s >>= 1;
        }
      }
      first_time--;
    }

    for (i=0;i<(nsamples/8);i++) {
      memcpy(data+16*i,lut[raw[i]],16*sizeof(float));
    }
}


float randu() {
// Uniform random numbers between 0 and 1
  static bool first=true;
  if (first) {
    srand(time(0));
    first=false;
  }
  return static_cast<float>(rand())/RAND_MAX;
}

float randn() {
// normally distributed random numbers
  static float last=0;
  float a,b,h=0;
  if (last==0) {
    while (h==0 || h>=1) {
      a=2*randu()-1;
      b=2*randu()-1;
      h=a*a+b*b;
    }
    h=sqrt(-2*log(h)/h);
    last=a*h;
    return b*h;
  } else {
    a=last;
    last=0;
    return a;
  }
}

int set_iDataIdx(int iDataIdx, int &iTapeBufferIdx, int iPrevTapeBufferIdx) {
    /* NOTE: for the last tape buffer, the last polyphase filtering we perform is for the
       last consecutive g_iNTaps blocks */
    /* rewind the data index by (g_iNTaps - 1) blocks */
    iDataIdx -= ((g_iNTaps - 1) * PFB_FFT_LEN);
    if (iTapeBufferIdx != iPrevTapeBufferIdx) {
        if (iDataIdx != 0) {  /* data index can only be negative or zero at this point */
            /* if rewinding takes us back to the previous tapebuffer,
               change the tapebuffer index and data index accordingly */
            iTapeBufferIdx = iPrevTapeBufferIdx;
            /* NOTE: data index is negative, hence the addition */
            iDataIdx = tapebuffer[iTapeBufferIdx].data.size() + iDataIdx;
        }
    }

    return(iDataIdx);
}

inline int inc_iDataIdx(int &iTapeBufferIdx, int &iDataIdx) {

    int iFlagBreak = FALSE;

    iDataIdx = (iDataIdx + 1) % tapebuffer[iTapeBufferIdx].data.size();
    if (0 == iDataIdx) {
        /* move to the next tapebuffer */
        ++iTapeBufferIdx;
        if (tapebuffer.size() == iTapeBufferIdx) {
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG, "End of tape buffer.\n");
            iFlagBreak = TRUE;
            /* NOTE: do not return here, as we still need to process the current block */
        }
    }

    return(iFlagBreak);
}

inline int load_pfb_data(int &iTapeBufferIdx, int &iDataIdx, int &i, int &k) {

    g_astPFBData[i].pcfData[k] = complex<float>(
        // The PFB has the opposite notion of i and q than that of the data recorder.
        // To account for this, we flip real and imag here.
        static_cast<float>(tapebuffer[iTapeBufferIdx].data[iDataIdx].imag()),
        static_cast<float>(tapebuffer[iTapeBufferIdx].data[iDataIdx].real())
    );

    return(inc_iDataIdx(iTapeBufferIdx, iDataIdx));
}

void reorder_data(fftwf_complex *pfcPackedDataOut, int iPFBTapIdx) {

    int i;

    /* re-order the output data in such a way that at the end of this loop,
       output is in the same form as before the PFB implementation,
       so output_samples() can be used the same way as before.
       (in the above output, frequency is the fastest-changing index,
       but output_samples() requires data with time as the fastest-changing
       index.) */
    for (i = 0; i < PFB_FFT_LEN; ++i) {
        // The PFB has the opposite notion of i and q than that of the workunit.
        // To account for this, we flip real and imag here.
        /* imaginary part of g_pfcFFTArray[][] */
        pfcPackedDataOut[(i*PFB_OUTPUT_STACK_SIZE)+iPFBTapIdx][0] = g_pfcFFTArray[i][1];
        /* real part of g_pfcFFTArray[][] */
        pfcPackedDataOut[(i*PFB_OUTPUT_STACK_SIZE)+iPFBTapIdx][1] = g_pfcFFTArray[i][0];

    }
}


void process_seg() {
    int iTapeBufferIdx = 0;
    signed int iDataIdx = 0;
    int iFlagBreak = FALSE;

    while (iTapeBufferIdx < tapebuffer.size()) {                        // walk tape blocks
        while (iDataIdx < tapebuffer[iTapeBufferIdx].data.size()) {     // walk data within a tape block
            int i;
            float* fbuff = (float*) g_pcfPackedDataOut;
            fftwf_complex *pfcPackedDataOut = (fftwf_complex*) g_pcfPackedDataOut;
            float r_total=0, r_mean=0, i_total=0, i_mean=0;     // used for nulling the DC bin
            int iPrevTapeBufferIdx = 0; /* to keep track of tapebuffer-index-change */
            int iPFBOutputStackIdx;
            int iPFBTapIdx;
            int iPFBFftIdx;

            // this outer loop just stacks up the amout of data required by output_samples()
            for(iPFBOutputStackIdx=0; iPFBOutputStackIdx<PFB_OUTPUT_STACK_SIZE; iPFBOutputStackIdx++) { 

                iPrevTapeBufferIdx = iTapeBufferIdx;   // save the current tapebuffer index, to check if it has changed in this read

                // load the data on which the PFB + FFT will operate
                for (iPFBTapIdx = 0; iPFBTapIdx < g_iNTaps; ++iPFBTapIdx) {
                    for (iPFBFftIdx = 0; iPFBFftIdx < PFB_FFT_LEN;iPFBFftIdx ++) {
                        iFlagBreak = load_pfb_data(iTapeBufferIdx, iDataIdx, iPFBTapIdx, iPFBFftIdx);
                    }  
                } 

                iDataIdx = set_iDataIdx(iDataIdx, iTapeBufferIdx, iPrevTapeBufferIdx);

                DoPFB(0);                       // use the PFB to filter data for FFT; 
                                                //  data set is g_iNTaps*PFB_FFT_LEN in size

                fftwf_execute(g_stPlan);        // this is an in-place FFT so output in g_pfcFFTArray itself;
                                                //  data set is now PFB_FFT_LEN in size

                reorder_data(pfcPackedDataOut, iPFBOutputStackIdx);

                if (iFlagBreak) break;
            }  // end stack data for output_samples()

            // effectively null the DC bin.  The initial complex data point in each spectra
            // of the output stack is time domain data for subband 0, which contains the DC bin.
            for(i=0; i<PFB_OUTPUT_STACK_SIZE; i++) {
                r_total += pfcPackedDataOut[i*PFB_FFT_LEN][0];         // (real part)
                i_total += pfcPackedDataOut[i*PFB_FFT_LEN][1];         // (imaginary part)
            }
            r_mean = r_total/PFB_OUTPUT_STACK_SIZE;
            i_mean = i_total/PFB_OUTPUT_STACK_SIZE;
            for(i=0; i<PFB_OUTPUT_STACK_SIZE; i++) {
                pfcPackedDataOut[i*PFB_FFT_LEN][0] -= r_mean;          // (real part) 
                pfcPackedDataOut[i*PFB_FFT_LEN][1] -= i_mean;          // (imaginary part) 
            }
            

            for (i=0; i<NSTRIPS; i++) {                 // NSTRIPS = 256 WUs in 1 WUG
                output_samples(fbuff, i);               // output_samples() outputs 8 complex samples per call
                fbuff += PFB_OUTPUT_STACK_SIZE*2;       // ..so, we move fbuff ahead by PFB_OUTPUT_STACK_SIZE (8) complex (via *2) floats
            }

            if (iFlagBreak) break;
        }  // end, walk data within a tape block 

        if (iFlagBreak) break;
    }  // end, walk tape blocks

    return;
}

void do_transform(std::vector<dr2_compact_block_t> &tapebuffer) {
    int iRet = EXIT_SUCCESS;

    /* initialise PFB-related stuff */
    if (g_iIsFirstRun)
    {
        iRet = InitPFB();
        if (iRet != EXIT_SUCCESS)
        {
       	    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "InitPFB() failed\n");
            PFBCleanUp();
            return;
        }
        log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG, "PFB initialised.\n");
        g_iIsFirstRun = FALSE;
    }

    process_seg();

   /* Move the data in the buffer so we don't have to reread portions of the
    * tape.  Leave a 20% overlap.
    */
   tapebuffer.erase(tapebuffer.begin(),tapebuffer.begin()+tapebuffer.size()*8/10);

}

/* function that allocates memory, reads filter coefficients, creates FFT plan,
   etc. */
int InitPFB()
{
    int iRet = EXIT_SUCCESS;
    int iFileCoeff = 0;
    int i = 0;

    g_iNTaps       = splitter_settings.splitter_cfg->pfb_ntaps;
    g_fWidthFactor = splitter_settings.splitter_cfg->pfb_width_factor;


    /* allocate memory for the filter coefficients */
    g_pfPFBCoeff = (float *) malloc(g_iNTaps 
                                    * PFB_FFT_LEN
                                    * sizeof(float));
    if (NULL == g_pfPFBCoeff)
    {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "InitPFB(): Memory allocation failed with %s\n", 
                       strerror(errno));
        return EXIT_FAILURE;
    }

    gen_coeff(g_pfPFBCoeff, g_iNTaps * PFB_FFT_LEN);	// generate the coefficients

    /* allocate memory for the PFB data arrays */
    g_astPFBData = (PFB_DATA *) malloc(g_iNTaps * sizeof(PFB_DATA)); 
    if (NULL == g_astPFBData)
    {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "InitPFB(): Memory allocation failed with %s\n", 
                       strerror(errno));
        return EXIT_FAILURE;
    }
    for (i = 0; i < g_iNTaps; ++i)
    {
        g_astPFBData[i].pcfData
            = (complex<float>*) fftwf_malloc(PFB_FFT_LEN * sizeof(complex<float>));
        if (NULL == g_astPFBData[i].pcfData)
        {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "InitPFB(): Memory allocation failed with %s\n", 
                           strerror(errno));
            return EXIT_FAILURE;
        }
        /* pointer of type fftwf_complex */
        g_astPFBData[i].pfcData = (fftwf_complex*) g_astPFBData[i].pcfData;
    }

    /* allocate memory for FFT input/output array */
    g_pfcFFTArray = (fftwf_complex *) fftwf_malloc(PFB_FFT_LEN
                                                 * sizeof(fftwf_complex));
    if (NULL == g_pfcFFTArray)
    {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "InitPFB(): Memory allocation failed with %s\n", 
                       strerror(errno));
        return EXIT_FAILURE;
    }

    /* create FFT plan */
    g_stPlan = fftwf_plan_dft_1d(PFB_FFT_LEN,
                                 g_pfcFFTArray,
                                 g_pfcFFTArray,
                                 FFTW_FORWARD,
#ifdef ANDROID
                FFTW_ESTIMATE
#else
                FFTW_MEASURE
#endif
    );

    /* allocate memory to pack the output of g_iNTaps FFTs in the PFB
       loop in the process_seg() function, so that the output data structure
       remains same as before PFB implementation */
    g_pcfPackedDataOut = (complex<float> *) fftwf_malloc(PFB_OUTPUT_STACK_SIZE
                                                         * PFB_FFT_LEN
                                                         * sizeof(complex<float>));
    if (NULL == g_pcfPackedDataOut)
    {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL, "InitPFB(): Memory allocation failed with %s\n", 
                       strerror(errno));
        return EXIT_FAILURE;
    }

    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "InitPFB(): Configured with NTaps %d WidthFactor %f\n", 
	      	   g_iNTaps, g_fWidthFactor);
    return EXIT_SUCCESS;
}

/* function that performs the PFB */
void DoPFB(int iPFBReadIdx)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int iCoeffStartIdx = 0;

    /* reset memory */
    (void) memset(g_pfcFFTArray, '\0', PFB_FFT_LEN * sizeof(fftw_complex));

    /* perform polyphase filtering, starting from the read index */
    i = iPFBReadIdx; 
    for (j = 0; j < g_iNTaps; ++j)
    {
        iCoeffStartIdx = j * PFB_FFT_LEN;
        for (k = 0; k < PFB_FFT_LEN; ++k)
        {
            /* real part */
            g_pfcFFTArray[k][0] += g_astPFBData[i].pfcData[k][0]
                                   * g_pfPFBCoeff[iCoeffStartIdx+k];
            /* imaginary part */
            g_pfcFFTArray[k][1] += g_astPFBData[i].pfcData[k][1]
                                   * g_pfPFBCoeff[iCoeffStartIdx+k];
        }
        i = (i + 1) % g_iNTaps;
    }

    return;
}

/* function that frees resources */
void PFBCleanUp()
{
    int i = 0;

    /* free resources */
    for (i = 0; i < g_iNTaps; ++i)
    {
        free(g_astPFBData[i].pcfData);
    }

    fftwf_free(g_pfcFFTArray);
    free(g_pfPFBCoeff);

    /* destroy plans */
    fftwf_destroy_plan(g_stPlan);

    fftwf_cleanup();

    return;
}

/*
 *
 * $Log: mb_dotransform.cpp,v $
 * Revision 1.1.2.3  2007/06/06 15:58:29  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.2  2007/04/25 17:27:30  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.1  2006/12/14 22:24:40  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/09/11 18:53:37  korpela
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/29 20:35:36  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.1  2003/06/03 00:16:10  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.2  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.1  2003/04/10 22:09:00  korpela
 * *** empty log message ***
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.7  1999/03/27 16:19:35  korpela
 * *** empty log message ***
 *
 * Revision 2.6  1999/02/22  19:02:50  korpela
 * Revered input real & imaginary.
 *
 * Revision 2.5  1999/02/10  21:49:44  korpela
 * Zeroed DC component of forward transform.
 *
 * Revision 2.4  1999/02/01  22:28:52  korpela
 * FFTW version.
 *
 * Revision 2.3  1998/12/14  23:41:44  korpela
 * *** empty log message ***
 *
 * Revision 2.2  1998/11/04 23:08:25  korpela
 * Byte and bit order change.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.1  1998/10/27  00:51:08  korpela
 * Initial revision
 *
 *
 */

