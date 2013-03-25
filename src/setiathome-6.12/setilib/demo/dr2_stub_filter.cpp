#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "setilib.h"

int main (int argc, char ** argv) {

    const char *usage = "usage: dr2_stub_filter input_data_file output_data_file\n  will read in raw data from input_data_file and write it out to output_data_file\n";

    int inputfd;
    int rbfd;
    int outputfd;
    char inputfilename[256];
    char outputfilename[256];

    char *inputbuffer=(char *)calloc(BufSize,1);
    size_t readbytes,writebytes;
    dataheader_t dataheader;

    if (argc != 3) {
      fprintf(stderr, usage); 
      exit(1);
      }

    strcpy(inputfilename,argv[1]);
    strcpy(outputfilename,argv[2]);

    if ((inputfd = open(inputfilename,O_RDONLY)) == -1) {
      fprintf(stderr,"couldn't open input data file for reading: %s\n",inputfilename);
      fprintf(stderr, usage); 
      exit(1);
      }
    if ((outputfd = open(outputfilename,O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)) == -1) {
      fprintf(stderr,"couldn't open output data file for writing: %s\n",outputfilename);
      fprintf(stderr, usage); 
      exit(1);
      }

    while ((readbytes = read(inputfd,inputbuffer,BufSize)) == BufSize) {
      dataheader.populate_from_data(inputbuffer);
      fprintf(stderr,"dsi: %d dataseq: %ld\n",dataheader.dsi,dataheader.dataseq);
      // do something with the data here if you want
      if ((writebytes = write(outputfd,inputbuffer,BufSize)) != BufSize) {
        fprintf(stderr,"error writing to file: %s - exiting!\n",outputfilename);
        exit(1);
        }
      }

}
