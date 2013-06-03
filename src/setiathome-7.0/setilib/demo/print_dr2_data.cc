#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "setilib.h"

int main(int argc, char ** argv) {

    int fd;
    int retval;
    long i, numcomplex;
    int channel, sample;
    long num_samples = 1024;
    long total_channels = 16;
    long num_channels = 0;
    long select_i = 0;
    bool graphics = false;
    bool show_blanking_signal = false;
    bool minus_one = false;
    enum get_by_t {CHANNEL, INPUT};
    get_by_t get_by;
    char * data_file;
    int channels[total_channels];
    int labels[total_channels];

    std::vector<complex<int> > cs;
    std::vector<int>           cw;
    std::vector<bool>          rb;

    typedef struct {
	int i;
	int q;
    } iq_t;
    iq_t * iq;

    for (i=1, select_i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-g")) {
            graphics = true;
        } else if (!strcmp(argv[i], "-b")) {
            show_blanking_signal = true;
        } else if (!strcmp(argv[i], "-c")) {
            get_by = CHANNEL;
        } else if (!strcmp(argv[i], "-i")) {
            get_by = INPUT;
        } else if (!strcmp(argv[i], "-f")) {
            data_file = argv[++i];
        } else if (!strcmp(argv[i], "-n")) {
            num_samples = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-1")) {
            minus_one = true;
        } else if (!strcmp(argv[i], "-s")) {
            channels[select_i++] = atoi(argv[++i]);
            num_channels++;
        } else {
            fprintf(stderr, "usage : print_dr2_data -f filename -n numsamples [-c -i -b -g -s selected channel -s selected channel ...]\n");
            exit(1);
        }
    }

    if (!num_channels) {
        num_channels = 16;
        for (i=0; i<num_channels; i++) {
            channels[i] = i;
            if (get_by == INPUT) channels[i]++;
        }
    }
    for (i=0; i<num_channels; i++) labels[i] = channels[i];

    iq = (iq_t *)calloc(sizeof(iq_t), num_samples*num_channels);

    switch (get_by) {
	case CHANNEL :
		break;
	case INPUT :
        for (i=0; i<num_channels; i++) {
		    fprintf(stderr, "translating input %d ", channels[i]);
		    //channels[i] = input_to_channel[i+1];
		    channels[i] = input_to_channel[channels[i]];
		    fprintf(stderr, "to channel %d\n", channels[i]);
        }
		break;
    }


    for (channel=0; channel < num_channels; channel++) {
    	if ((fd = open(data_file, O_RDONLY)) == -1) {
        	perror("Error opening data device\n");
        	exit(1);
    	} 
    	retval = seti_GetDr2Data(fd, channels[channel], num_samples, cs, NULL, cw, show_blanking_signal);
        if(!minus_one) {
	        for (sample=0; sample < num_samples; sample++) {
		        (*(iq+sample*num_channels+channel)).i = cs[sample].real() == 1 ? 1 : 0; 
		        (*(iq+sample*num_channels+channel)).q = cs[sample].imag() == 1 ? 1 : 0; 
	        }
        } else {
	        for (sample=0; sample < num_samples; sample++) {
		        (*(iq+sample*num_channels+channel)).i = cs[sample].real(); 
		        (*(iq+sample*num_channels+channel)).q = cs[sample].imag(); 
	        }
        }
	    close(fd);
	    cs.clear();
    }

    if(!retval) {
        fprintf(stderr, "Could not allocate or fill array\n");
        exit(1);
    } 

#define PNTS_PR_SCRN 64
int pnts;

    if (graphics) {
        for (sample = 0; sample < num_samples; sample++) {
            system("clear");
    	    for (channel=0;  channel < num_channels; channel++) {
		        //fprintf(stderr, "%2di ", get_by == CHANNEL ? channel : channel+1);
		        fprintf(stderr, "%2di ", labels[channel]);
		        for (pnts=0; pnts < PNTS_PR_SCRN; pnts++) {
			        if ( (*(iq +((sample+pnts)*num_channels) +(channel))).i == 0) {
				        fprintf(stderr, "_");
			        } else {
				        fprintf(stderr, "-");
			        }
		        }
		        fprintf(stderr, "\n");
		        //fprintf(stderr, "%2dq ", get_by == CHANNEL ? channel : channel+1);
		        fprintf(stderr, "%2dq ", labels[channel]);
		        for (pnts=0; pnts < PNTS_PR_SCRN; pnts++) {
			        if ( (*(iq +((sample+pnts)*num_channels) +(channel))).q == 0) {
				        fprintf(stderr, "_");
			        } else {
				        fprintf(stderr, "-");
			        }
		        }
		        fprintf(stderr, "\n\n");
	        }   
            getchar();
        }
    } else {                        // no graphics
        for (sample = 0; sample < num_samples; sample++) {
       	    fprintf(stderr,"%8ld ",sample);
	        for (channel=0; channel < num_channels; channel++) {
        	    fprintf(stderr,"%3d%3d ",
                    (*(iq+sample*num_channels+channel)).i, (*(iq+sample*num_channels+channel)).q);
	        }
            if (show_blanking_signal) {
                fprintf(stderr," %2d ", cw[sample]);
            }
       	    fprintf(stderr,"\n");
        }
    }
}
