#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "setilib.h"
//#include "seti_dr2filter.h"

int main(int argc, char ** argv) {

    int fd;
    //int * cs;
    int i, j, retval;
    long numblocks;
    int channel;

    if ((fd = open(argv[1],O_RDONLY)) == -1) {
        perror("Error opening data device\n");
        exit(1);
    }

    std::vector<dr2_block_t> dr2_block;
    std::vector<bool> radar_blank_signal;
    channel    = atoi(argv[2]);
    numblocks = atoi(argv[3]);

    retval = seti_StructureDr2Data(fd, (telescope_id)channel, numblocks, dr2_block, randomize); 
    //retval = seti_StructureDr2Data(fd, (telescope_id)channel, numblocks, dr2_block, direct_subtract); 

    if(retval < numblocks) {
        fprintf(stderr, "Could not allocate or fill array\n");
        exit(1);
    } 

    fprintf(stdout, "dr2_block is %d elements long\n", dr2_block.size());

    for (i = 0; i < (int)dr2_block.size(); i++) {
        for (j = 0; j < (int)dr2_block[i].data.size(); j++) {
            fprintf(stdout,"%ld %f %f\n",j, dr2_block[i].data[j].real(), 
                                            dr2_block[i].data[j].imag());
        }
    }
}
