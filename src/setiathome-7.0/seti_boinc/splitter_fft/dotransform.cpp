/*
 * 
 * dotransform.c
 *
 * Functions for division by frequency into work units.
 *
 * $Id: dotransform.cpp,v 1.2.4.1 2006/12/14 22:24:38 korpela Exp $
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

#include "splitparms.h"
#include "splittypes.h"
#include "splitter.h"
#include "fftw.h"
#include "wufiles.h"

/* buffer for fft input/output */
float databuf[FFT_LEN*2];

/* buffer for ftt output before data writes, needs to be multiple
 * of three bytes long for encode to work.
 */
#define SAMPLES_PER_OBUF (FFT_LEN*2*3/CHAR_BIT)
unsigned char output_buf[NSTRIPS][SAMPLES_PER_OBUF];  


int obuf_pos; /* Tracks current postition in the output buffers */


void output_samples(float *data, int i, int buf_pos) {
  int j,k;
  unsigned short s;
  float *p=data;

  for (j=0;j<2;j++) {
    s=0;
    for (k=0; k<8; k++) {
        s >>= 2;
        if (*p>0) s |= 0x8000;
	if (*(p+1)>0) s |= 0x4000;
        p+=2;
    }
    output_buf[i][buf_pos++]=*(char *)(&s);
    output_buf[i][buf_pos++]=*(((char *)(&s))+1);
  }
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

void process_seg(float* data) {
    int i;
    float* p = data;
    static float dbuff[FFT_LEN*2];
    static fftw_plan  planfwd,planinverse;

    if (!planfwd) {
      planfwd=fftw_create_plan(FFT_LEN, FFTW_BACKWARD, 
#ifdef ANDROID
                FFTW_ESTIMATE
#else
		FFTW_MEASURE
#endif
		| FFTW_IN_PLACE | FFTW_USE_WISDOM );
      planinverse=fftw_create_plan(IFFT_LEN, FFTW_FORWARD,
#ifdef ANDROID
                FFTW_ESTIMATE
#else
                FFTW_MEASURE
#endif
		| FFTW_IN_PLACE | FFTW_USE_WISDOM );
    }

    fftw_one(planfwd, (fftw_complex *)data, (fftw_complex *)NULL);
    data[0]=0;
    data[1]=0;
    fftw(planinverse, NSTRIPS, (fftw_complex *)data, 1, IFFT_LEN, 
  			(fftw_complex *)NULL, 1, IFFT_LEN);
    for (i=0; i<NSTRIPS; i++) {
        output_samples(p, i, obuf_pos);
        p += IFFT_LEN*2;
    }
    obuf_pos+=IFFT_LEN*2/CHAR_BIT;
}

#define TBUF_OFFSET(frame,byte) (tapebuffer+(frame)*TAPE_FRAME_SIZE+(byte)+TAPE_HEADER_SIZE)

void do_transform(buffer_pos_t *start_of_wu, buffer_pos_t *end_of_wu) {
   buffer_pos_t end_trans,start_trans=*start_of_wu;
   int i,nsamp;
   do {
     obuf_pos=0;  /* reset to the beginning of the output buffer */
     for (i=0;i<768;i++) {
       end_trans.frame=start_trans.frame;
       end_trans.byte=start_trans.byte+(FFT_LEN*2/CHAR_BIT);
       if (end_trans.byte > TAPE_DATA_SIZE) {
       /* End of frame crossed need to trasfer correctly */
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
       process_seg(databuf);
       /* Go on to next transform */
       start_trans=end_trans;
     }
     /* Check if we're at the end of the wu file. If so we print less bytes */
     if ((end_trans.frame>=end_of_wu->frame) && 
			 (end_trans.byte>=end_of_wu->byte)) {
       write_wufile_blocks(NBYTES % SAMPLES_PER_OBUF);
     } else {
       write_wufile_blocks(SAMPLES_PER_OBUF);
     }
   } while (!((end_trans.frame>=end_of_wu->frame) && (end_trans.byte>=end_of_wu->byte)));

   /* Move the data in the buffer so we don't have to reread portions of the
    * tape.
    */
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

/*
 *
 * $Log: dotransform.cpp,v $
 * Revision 1.2.4.1  2006/12/14 22:24:38  korpela
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
