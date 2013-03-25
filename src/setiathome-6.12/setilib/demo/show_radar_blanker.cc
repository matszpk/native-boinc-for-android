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
    enum get_by_t {CHANNEL, INPUT};
    get_by_t get_by;
    char * data_file;
    int channels[total_channels];
    int labels[total_channels];
    double high_count=0.0, low_count=0.0;

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
	    for (sample=0; sample < num_samples; sample++) {
		    (*(iq+sample*num_channels+channel)).i = cs[sample].real() == 1 ? 1 : 0; 
		    (*(iq+sample*num_channels+channel)).q = cs[sample].imag() == 1 ? 1 : 0; 
	    }
	    close(fd);
	    cs.clear();
    }

    if(!retval) {
        fprintf(stderr, "Could not allocate or fill array\n");
        exit(1);
    } 

    for (sample = 0; sample < num_samples; sample++) {
        if ((int)rb[sample] == 0) {
            if(high_count != 0) {
                printf("HIGH %lf ms\n", (high_count/2500000) * 1000);
                high_count = 0;
            }
            low_count++;
        } else {    // sample == 1  
            if(low_count != 0) {
                printf("LOW  %lf ms\n", (low_count/2500000) * 1000);
                low_count = 0;
            }
            high_count++;
        }
    }
}
