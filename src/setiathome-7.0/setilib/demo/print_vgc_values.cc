#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "setilib.h"

int main(int argc, char ** argv) {

    int i, fd, retval, vgc, channel, buf, num_bufs;
    char * data_file;
    dataheader_t header;
    bool print_ra = false;

    std::vector<complex<int> > cs;
    std::vector<int> cw;
    std::vector<bool>          rb;

    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-c")) {
            channel = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-n")) {
            num_bufs = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-f")) {
            data_file = argv[++i];
        } else if (!strcmp(argv[i], "-ra")) {
            print_ra = true;
        } else {
            fprintf(stderr, "usage : print_vgc_values -f filename -c channel -n num_bufs [-ra] \n");
            exit(1);
        }
    }

   	if ((fd = open(data_file, O_RDONLY)) == -1) {
       	perror("Error opening data device\n");
       	exit(1);
   	} 

    for (buf=0; buf < num_bufs; buf++) {
    	retval = seti_GetDr2Data(fd, channel, DataWords, cs, &header, cw, false);
        if(!retval) {
            fprintf(stderr, "Could not allocate or fill array\n");
            exit(1);
        } 
        // fprintf(stderr,"test: %d\n",header.channel);
        vgc = get_vgc_for_channel(channel, header);
        if(vgc < 0) {
            fprintf(stderr, "Bad channel");
            exit(1);
        } else {
            fprintf(stderr, "vgc %d : %d", buf+1, vgc);
            if (print_ra) fprintf(stderr, " ra : %lf",header.ra); 
        }
        fprintf(stderr,"\n");
    }
}
