#define NONE    0
#define WELCH   1
#define HANNING 2

#define N_WINDOWS 8
#define P_FFT_LEN 256

/* buffer for fft input/output */
extern float databuf[FFT_LEN*2];

/* buffer for ftt output before data writes, needs to be multiple
 * of three bytes long for encode to work.
 */
#define SAMPLES_PER_OBUF (FFT_LEN*2*3/CHAR_BIT)
#define TBUF_OFFSET(frame,byte) (tapebuffer+(frame)*TAPE_FRAME_SIZE+(byte))
extern unsigned char output_buf[NSTRIPS][SAMPLES_PER_OBUF];  

extern double *filter_r, *filter_i;
extern float *f_data;

extern int obuf_pos; /* Tracks current postition in the output buffers */

void make_FIR (int n_points, int M, int window, double *output);
void polyphase_seg(float *data);
void do_polyphase(buffer_pos_t *start_of_wu, buffer_pos_t *end_of_wu);

