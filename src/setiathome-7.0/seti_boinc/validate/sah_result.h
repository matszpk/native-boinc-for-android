#ifndef _SAH_RESULT_
#define _SAH_RESULT_

// this file should not refer to either BOINC or S@h DB

#include <stdio.h>
#include <vector>

using namespace std;

#define SIGNAL_TYPE_SPIKE            1
#define SIGNAL_TYPE_GAUSSIAN         2
#define SIGNAL_TYPE_PULSE            3
#define SIGNAL_TYPE_TRIPLET          4
#define SIGNAL_TYPE_BEST_SPIKE       5
#define SIGNAL_TYPE_BEST_GAUSSIAN    6
#define SIGNAL_TYPE_BEST_PULSE       7
#define SIGNAL_TYPE_BEST_TRIPLET     8
#define SIGNAL_TYPE_AUTOCORR         9
#define SIGNAL_TYPE_BEST_AUTOCORR    10

// Result Flags.  Can be passed from validator to assimilator
// via the BOINC result.opaque field.  Note that you have
// to cast to float in order to assign a flag to the opaque
// field.  OR into a temp int and then assign with a cast.
#define RESULT_FLAG_OVERFLOW 0x00000001

struct SIGNAL {
    int type;
    double power;
    double period;
    double ra;
    double decl;
    double time;
    double freq;
    double delay;
    double sigma;
    double chisqr;
    double max_power;
    double peak_power;
    double mean_power;
    double score;
    unsigned char pot[256];
    int len_prof;
    double snr;
    double thresh;
    int fft_len;
    double chirp_rate;

    bool checked;   // temp

    bool roughly_equal(SIGNAL&);
};

struct SAH_RESULT {
    bool have_result;
    bool overflow;
    vector<SIGNAL> signals;

    bool has_roughly_equal_signal(SIGNAL&);
    bool strongly_similar(SAH_RESULT&);
    bool weakly_similar(SAH_RESULT&);
    bool bad_values();
    int parse_file(FILE*);
    int num_signals;
};

extern int write_sah_db_entries(SAH_RESULT&);

#endif
