/*
 * 
 * dotransform.h
 *
 * Functions for division by frequency into work units.
 *
 * $Id: mb_dotransform.h,v 1.1.2.2 2007/04/25 17:20:28 korpela Exp $
 *
 */


/* buffer for ftt output before data writes, needs to be multiple
 * of three bytes long for encode to work.
 */
#define SAMPLES_PER_OBUF (FFT_LEN*2*3/CHAR_BIT)
#define TBUF_OFFSET(frame,byte) (tapebuffer+(frame)*TAPE_FRAME_SIZE+(byte))


extern int obuf_pos; /* Tracks current postition in the output buffers */


void output_samples(float *data, int i, int buf_pos);
void splitter_bits_to_float(unsigned short *raw, float *data, int nsamples) ;
void process_seg(std::vector<signed char> &data,int offset) ;
void do_transform(std::vector<dr2_compact_block_t> &tapebuffer) ;

/*
 *
 * $Log: mb_dotransform.h,v $
 * Revision 1.1.2.2  2007/04/25 17:20:28  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.1  2006/12/14 22:24:41  korpela
 * *** empty log message ***
 *
 * Revision 1.1  2003/06/03 00:16:11  korpela
 *
 * Initial splitter under CVS control.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.3  1999/02/01 22:28:52  korpela
 * FFTW version.
 *
 * Revision 2.2  1998/11/04  23:08:25  korpela
 * Byte and bit order change.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/27  01:05:22  korpela
 * Initial revision
 *
 *
 */
