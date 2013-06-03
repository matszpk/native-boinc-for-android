#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "setilib.h"

int main(int argc, char ** argv) {

    int             fd;
    int             num_headers, header_count, dsi, count_this_dsi, prior_dsi=-1, channel;
    int             num_missed=0, prior_missed=0;
    int             prior_dataseq, this_dataseq;
    char            buf[BufSize];   
    dataheader_t    dataheader;
    double          this_data_time, prior_data_time, time_diff;
    char            time_diff_str[64];
    char            error_msg[256];

    if ((fd = open(argv[1],O_RDONLY)) == -1) {
        sprintf(error_msg, "Error opening data device %s", argv[1]);
        perror(error_msg);
        exit(1);
    }

    num_headers = atoi(argv[2]);
    channel     = atoi(argv[3]);
    dsi         = channel > 7 ? 0 : 1;

    for(header_count = 0; 
        header_count < num_headers; 
        header_count++, prior_dataseq = this_dataseq) {

        if ((unsigned)read(fd, buf, BufSize) != BufSize) break;
        dataheader.populate_from_data(buf);
        dataheader.populate_derived_values(channel);
        if(dataheader.dsi != dsi) continue;

        this_dataseq = dataheader.dataseq;
        if(header_count == 0) { 
            this_data_time = dataheader.data_time.jd().uval();
            prior_dataseq = 0;
        }            

        if(dataheader.missed) {
            num_missed += (this_dataseq-prior_dataseq)-1;
        }    

        if(dataheader.dsi != prior_dsi) {
            prior_missed    = num_missed;
            num_missed      = 0;
            count_this_dsi  = 0;
            prior_dsi       = dataheader.dsi;
            prior_data_time = this_data_time;
            this_data_time  = dataheader.data_time.jd().uval();
            time_diff       = this_data_time - prior_data_time;
            sprintf(time_diff_str, "time diff is %lf", time_diff*86400.0); 
        } else {
            sprintf(time_diff_str, ""); 
        }     

        printf("HEADER %d for dsi %d  dataseq %d %d  data_time  %lf  prior missed %d  %s\n", ++count_this_dsi, dataheader.dsi, dataheader.dataseq, dataheader.dataseq-prior_dataseq, this_data_time, prior_missed, time_diff_str);
        dataheader.print();
        prior_dataseq = dataheader.dataseq;
    }
}
