#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "setilib.h"
#include <complex>

synthetic_data::synthetic_data(double thisfreq, double thisamp, double thisduration, double thischirp, double thisgauss_midpt, double thisgauss_sigma, double thispulse_period, double thispulse_duty, double thissample_rate) {

    freq = thisfreq;
    amp = thisamp;
    duration = thisduration;
    chirp = thischirp;
    gauss_midpt = thisgauss_midpt;
    gauss_sigma = thisgauss_sigma;
    pulse_period = thispulse_period; 
    pulse_duty = thispulse_duty;
    sample_rate = thissample_rate;

    signal.clear();

    int npoints, cnt, i, j;
    unsigned int c;
    double real, imag, mytime;

    npoints = (int)(sample_rate * 1000000 * duration);
    cnt = 0;

    double sine_phase = 0, sine_freq;
    double dt = duration/npoints;
    double sigma_sq = gauss_sigma*gauss_sigma;
    double pulseActive = 1.0;
    double pulseTime = pulse_period*pulse_duty/100.0;
    complex<float> tempcomplex;

    for (i=0; i<npoints; i++) {
        real = imag = 0;
        mytime = (duration*i)/npoints;
        double ratio = i/(double)npoints;
        pulseActive = 1.0;
        if (pulse_period != -1)
            if( (mytime - (floor(mytime/pulse_period))*pulse_period) > pulseTime ) pulseActive = 0.0;
        if (amp) {
            real += amp*cos(sine_phase)*pulseActive;
            imag -= amp*sin(sine_phase)*pulseActive;
            freq = freq + chirp*mytime;
            sine_phase += fmod(M_PI*2*dt*freq,M_PI*2);
            sine_phase = fmod(sine_phase,M_PI*2);
          }
        if (gauss_midpt != -1) {
            double gauss_factor = EXP(mytime, gauss_midpt, sigma_sq);
            real *= gauss_factor*pulseActive;
            imag *= gauss_factor*pulseActive;
          }
       tempcomplex.real() = (float)real;
       tempcomplex.imag() = (float)imag;
       signal.push_back(tempcomplex);
    } // end for

  } // end constructor
