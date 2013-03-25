
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
#include "mb_fftw.h"
#include "mb_wufiles.h"
#include "mb_polyphase.h"
#include "mb_dotransform.h"

/* buffer for fft input/output */
//float databuf[FFT_LEN*2];

/* buffer for ftt output before data writes, needs to be multiple
 * of three bytes long for encode to work.
 */
#define SAMPLES_PER_OBUF (FFT_LEN*2*3/CHAR_BIT)

extern unsigned char output_buf[NSTRIPS][SAMPLES_PER_OBUF];

double *filter_r, *filter_i;
float *f_data;

extern int obuf_pos; /* Tracks current postition in the output buffers */

void make_FIR (int n_points, int M, int window, double *output) {
  /* Create Mth band lowpass FIR filter, n_points long. */

  /* Modify this to give 8-bit quantized filter. */
  /* Also generate Hilbert transformed filter */

  int n;
  double q, p;

  for (n=0; n<n_points; n++) {
    if (n == n_points/2) {
      output[n] = 1;
    } else {
      q = n-(n_points/2);
      p = n_points/M;
      output[n] = sin(M_PI*q/p)/(M_PI*q/p);
    }
  }
  if (window == HANNING) {
    for (n=0; n<n_points; n++) {
      output[n] = output[n]*.5*(1 - cos(2*M_PI*n/(n_points-1)));
    }
  }
  else if (window == WELCH) {
    for (n=0; n<n_points; n++) {
      output[n] = output[n]*(1 - ((n-.5*(n_points-1))/(.5*(n_points+1)))*((n-.5*(n_points-1))/(.5*(n_points+1))));
    }
  }
}


void polyphase_seg(float* data) {
    int i,n;
    static fftw_plan planfwd;
    float *p;

    if (!planfwd) {
      planfwd=fftw_create_plan(P_FFT_LEN, FFTW_FORWARD, 
		FFTW_MEASURE | FFTW_IN_PLACE | FFTW_USE_WISDOM );
    }

    for (i=0;i<P_FFT_LEN;i++) {
        f_data[2*i] = 0;
        f_data[2*i+1] = 0;
        for (n=0;n<N_WINDOWS;n++) {
            f_data[2*i] += data[2*i+2*n*P_FFT_LEN]*filter_r[P_FFT_LEN*n + i];
            f_data[2*i+1] += data[2*i+2*n*P_FFT_LEN+1]*filter_i[P_FFT_LEN*n + i];
        }
    }
    fftw_one(planfwd, (fftw_complex *)f_data, (fftw_complex *)NULL);
    /*for (i=0;i<P_FFT_LEN;i++) {
        fprintf( stderr, "%f %f\n", f_data[2*i], f_data[2*i+1]);
    }*/
    //fprintf( stderr, "%f %f ", f_data[2*3], f_data[2*3+1] );
    p = f_data;
    for (i=0; i<P_FFT_LEN; i++) {
        output_samples(p, i, obuf_pos);
        p += IFFT_LEN*2;
    }
    obuf_pos+=IFFT_LEN*2/CHAR_BIT;
}

#define TBUF_OFFSET(frame,byte) (tapebuffer+(frame)*TAPE_FRAME_SIZE+(byte)+TAPE_HEADER_SIZE)

void do_polyphase(buffer_pos_t *start_of_wu, buffer_pos_t *end_of_wu) {
   buffer_pos_t end_trans,start_trans=*start_of_wu;
   int i,nsamp;
   do {
     obuf_pos=0;  // reset to the beginning of the output buffer
     for (i=0;i<768;i++) {
       end_trans.frame=start_trans.frame;
       end_trans.byte=start_trans.byte+(FFT_LEN*2/CHAR_BIT);
       if (end_trans.byte > TAPE_DATA_SIZE) {
       // End of frame crossed need to trasfer correctly
          end_trans.frame++;
          end_trans.byte-=TAPE_DATA_SIZE;
          nsamp=(TAPE_DATA_SIZE-start_trans.byte)*(CHAR_BIT/2);
          assert((nsamp+end_trans.byte*(CHAR_BIT/2)) == FFT_LEN);
          splitter_bits_to_float((unsigned short *)TBUF_OFFSET(start_trans.frame,start_trans.byte),
                        databuf, nsamp);
          splitter_bits_to_float((unsigned short *)TBUF_OFFSET(end_trans.frame,0),databuf+nsamp*2,
                        end_trans.byte*(CHAR_BIT/2));
       } else {
          splitter_bits_to_float((unsigned short *)TBUF_OFFSET(start_trans.frame,start_trans.byte),
                        databuf,FFT_LEN);
       }
       polyphase_seg(databuf);
       // Go on to next transform
       start_trans=end_trans;
     }
     // Check if we're at the end of the wu file. If so we print less bytes
     if ((end_trans.frame>=end_of_wu->frame) && 
			 (end_trans.byte>=end_of_wu->byte)) {
       write_wufile_blocks(NBYTES % SAMPLES_PER_OBUF);
     } else {
       write_wufile_blocks(SAMPLES_PER_OBUF);
     }
   } while (!((end_trans.frame>=end_of_wu->frame) && (end_trans.byte>=end_of_wu->byte)));

   // Move the data in the buffer so we don't have to reread portions of the
   // tape.
   //
   { 
     unsigned char *record_offset;
     int copysize;
     end_of_wu->frame-=WU_OVERLAP_FRAMES;
     records_in_buffer=TAPE_RECORDS_IN_BUFFER-
			 (end_of_wu->frame/TAPE_FRAMES_PER_RECORD);
     record_offset=tapebuffer+
             (end_of_wu->frame/TAPE_FRAMES_PER_RECORD)*TAPE_RECORD_SIZE;
     copysize=records_in_buffer*TAPE_RECORD_SIZE;
     bcopy(record_offset,tapebuffer,copysize);
   }
}


