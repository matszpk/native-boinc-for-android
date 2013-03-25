#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define NEG_LN_ONE_HALF     0.693
#define EXP(a,b,c)          exp(-(NEG_LN_ONE_HALF * pow((double)(a) - (double)(b), 2.0)) / (double)(c))

class synthetic_data {

    private:
    double freq;
    double amp;
    double duration;
    double chirp;
    double gauss_midpt;
    double gauss_sigma;
    double pulse_period;
    double pulse_duty;
    double sample_rate;

    public:
    std::vector< complex<float> > signal;
    synthetic_data(double thisfreq, double thisamp, double thisduration, double thischirp, double thisgauss_midpt, double thisguass_sigma, double thispulse_period, double thispulse_duty, double thissample_rate);

    };

