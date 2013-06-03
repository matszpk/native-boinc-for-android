#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <complex.h>
#include "setilib.h"

int main(int argc, char ** argv) {

    int fd, retval, num_samples_read;
    long i, numcomplex;
    int channel, sample;
    long num_samples = 1024;
    long total_channels = 16;
    long num_channels = 0;
    long select_i = 0;
    bool graphics = false;
    bool do_blanking = false;
    bool print_line_numbers = false;
    enum get_by_t {CHANNEL, INPUT};
    get_by_t get_by;
    char * data_file;
    int channels[total_channels];
    int labels[total_channels];

    std::vector<dr2_compact_block_t> dr2_data;
    std::vector<std::vector<dr2_compact_block_t> > dr2_data_all;
    waveform_filter<complex<signed char> > filter;

    typedef struct {
	int i;
	int q;
    } iq_t;
    iq_t * iq;

    for (i=1, select_i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-g")) {
            graphics = true;
        } else if (!strcmp(argv[i], "-l")) {
            print_line_numbers = true;
        } else if (!strcmp(argv[i], "-b")) {
            do_blanking = true;
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

    if (do_blanking) {
        filter = randomize;
    } else {
        filter = NULL;
    }

 	if ((fd = open(data_file, O_RDONLY)) == -1) {
        perror("Error opening data device\n");
        exit(1);
   	} 


   	num_samples_read =  seti_StructureDr2Data(fd, channels[0], num_samples, dr2_data, filter);
    fprintf(stderr, "read %d of %d samples\n", num_samples_read, num_samples);
    close(fd);

    if(!num_samples_read) {
        fprintf(stderr, "Could not allocate or fill array\n");
        exit(1);
    } 

    std::vector<dr2_compact_block_t>::iterator dr2_i;
    std::vector<complex<signed char> >::iterator dr2_data_i;
    sample = 0;
    for (dr2_i = dr2_data.begin(); dr2_i < dr2_data.end(); dr2_i++) {
        for (dr2_data_i = dr2_i->data.begin(); 
             dr2_data_i < dr2_i->data.end() && sample++ < num_samples;  
             dr2_data_i++) {
   	        if (print_line_numbers) fprintf(stdout,"%8ld ",sample);
            fprintf(stdout,"%2d %2d\n", dr2_data_i->real(), dr2_data_i->imag());
        }
    }
}
