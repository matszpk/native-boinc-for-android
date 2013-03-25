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

#include "setilib.h"
#include "splitparms.h"
#include "splittypes.h"
#include "mb_splitter.h"
#include "fftw3.h"
#include "mb_wufiles.h"

/* buffer for ftt output before data writes, needs to be multiple
 * of three bytes long for encode to work.
 */
#define SAMPLES_PER_OBUF (FFT_LEN*2*3/CHAR_BIT)


int obuf_pos; /* Tracks current postition in the output buffers */


void output_samples(float *data, int i, int buf_pos) {
  int j,k;
  unsigned short s=0;
  float *p=data;

  for (k=0; k<8; k++) {
      s >>= 2;
      if (*p>0) s |= 0x8000;
      if (*(p+1)>0) s |= 0x4000;
      p+=2;
  }
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

void process_seg(std::vector<complex<signed char> > &data,int offset) {
    int i;
    static complex<float> *dbuff=(complex<float> *)fftwf_malloc(FFT_LEN*sizeof(complex<float>));
    memset(dbuff,0,FFT_LEN*sizeof(complex<float>));
    float* fbuff = (float *)dbuff;
    fftwf_complex *fcbuff=(fftwf_complex *)dbuff;
    static fftwf_plan  planfwd,planinverse;


    if (!planfwd) {
      planfwd=fftwf_plan_dft_1d(FFT_LEN, fcbuff, fcbuff, FFTW_BACKWARD, 
#ifdef ANDROID
        FFTW_ESTIMATE
#else
		FFTW_MEASURE
#endif
        );
      int n=IFFT_LEN;
      planinverse=fftwf_plan_many_dft(1, &n, FFT_LEN/IFFT_LEN,
                fcbuff, &n, 1, n,
		fcbuff, &n, 1, n,
		FFTW_FORWARD,
#ifdef ANDROID
        FFTW_ESTIMATE
#else
        FFTW_MEASURE
#endif
        );
    }

    for (i=0;i<FFT_LEN;i++) {
      dbuff[i]=complex<float>(
                static_cast<float>(data[i+offset].real()),
                static_cast<float>(data[i+offset].imag())
		);
    }

    fftwf_execute(planfwd);
    dbuff[0]=complex<float>(0.0f,0.0f); // null the DC bin


    // highpass filter the rest
    if (splitter_settings.splitter_cfg->highpass != 0) {
      float filter_bins=FFT_LEN*splitter_settings.splitter_cfg->highpass/splitter_settings.recorder_cfg->sample_rate;
      complex<float> stddev=0,avg=0;
      for (i=0;i<FFT_LEN;i++) {
       complex<float> temp(dbuff[i]);
       avg+=temp;
       stddev+=complex<float>(temp.real()*temp.real(),temp.imag()*temp.imag());
      }
      avg/=(float)FFT_LEN;
      stddev-=(float)FFT_LEN*complex<float>(avg.real()*avg.real(),avg.imag()*avg.imag());
      stddev=complex<float>(sqrt(stddev.real()),sqrt(stddev.imag()))/(float)FFT_LEN;
      for (i=0;i<FFT_LEN/2;i++) {
        float n=exp(-0.5*i*i/(filter_bins*filter_bins));
	float f=1-n;
	// subtract out the filter 
        dbuff[i]*=f;        
        dbuff[FFT_LEN-(i+1)]*=f;         
	// add in some noise to cover
	// for single bitting
	dbuff[i]+=complex<float>(randn()*stddev.real(),randn()*stddev.imag())*(n+0.25f);    
	dbuff[FFT_LEN-(i+1)]+=complex<float>(randn()*stddev.real(),randn()*stddev.imag())*(n+0.25f);    
      } 
    }

    dbuff[0]=complex<float>(0.0f,0.0f); // null the DC bin

    fftwf_execute(planinverse);
    for (i=0; i<NSTRIPS; i++) {
        output_samples(fbuff, i, obuf_pos);
        fbuff += IFFT_LEN*2;
    }
}

void do_transform(std::vector<dr2_compact_block_t> &tapebuffer) {
   int i,j;
   // long sum=0,sum0;
   for (i=0;i<tapebuffer.size();i++) {


     
     // sum+=tapebuffer[i].data.size();
     // fprintf(stderr,"input size: %ld   sum: %ld samples: %ld\n", tapebuffer[i].data.size(),sum, sum/2);
     // fflush(stderr);
     for (j=0;j<tapebuffer[i].data.size();j+=FFT_LEN) {
       process_seg(tapebuffer[i].data,j);
     }
     // sum0=0;
     // for (j=0;j<256;j++) sum0+=bin_data[i].size();
     // fprintf(stderr,"output size sum: %ld samples: %ld\n",sum0,sum0*4);
   }
   /* Move the data in the buffer so we don't have to reread portions of the
    * tape.  Leave a 20% overlap.
    */
   tapebuffer.erase(tapebuffer.begin(),tapebuffer.begin()+tapebuffer.size()*8/10);

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
