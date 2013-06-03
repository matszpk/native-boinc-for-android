#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include "setilib.h"

int main(int argc, char ** argv) {

    const int num_inputs = 16;
    int i, j, k, b, p, c, c2, dsi;
    //telescope_id tid;

    for (telescope_id tid=AO_ALFA_0_0; tid<AO_ALFA_6_1+1; ++tid) {
        c = receiverid_to_channel[tid];
        fprintf(stderr, "%d\n", c);
    }


    fprintf(stderr, " input    channel   dsi   beam   pol\n");
    // input numbering starts at 1
    for (i=1; i<num_inputs+1; i++) {
        c = input_to_channel[i];
        b = -1;
        for (j=0; j < 7 && b == -1; j++) {
            for (k=0; k < 2 && b == -1; k++) {
                c2 =  beam_pol_to_channel[bmpol_t(j,k)];
                if (c2 == c) {
                    b = j; 
                    p = k;
                }
            }
        }
        dsi = c > 7 ? 0 : 1;
        if (b == -1) {
            fprintf(stderr, "  %2d        %2d      %2d\n", i, c, dsi);
        } else {
            fprintf(stderr, "  %2d        %2d      %2d    %2d     %2d\n", i, c, dsi, b, p);
        }
    }
}
