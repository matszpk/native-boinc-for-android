#include "seti_config.h"
#include "fftw3.h"
#include "mtrand.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <complex>
#include <cstdlib>

using std::complex;
#include "seti_dr2filter.h"

// Finds (sum |arr1 - arr2|) / (sum max(|arr1|, |arr2|))
double absdiff_percent(double *arr1, double *arr2, int len) {
    double absdiff_total = 0;
    double abs_total = 0;
    for (int i = 0; i < len; i++) {
        absdiff_total += std::abs(arr1[i] - arr2[i]);
        abs_total += std::max(std::abs(arr1[i]), std::abs(arr2[i]));
    }
    printf("absdiff_total: %f, abs_total: %f\n", absdiff_total, abs_total);
    return absdiff_total / abs_total;
}

double gasdev(MtRand &mtr) {
    static int iset=0;
    static double gset;
    double fac, rsq, v1, v2;
    if (iset == 0) {
        do {
            v1=2.0*mtr.drand32()-1.0;
            v2=2.0*mtr.drand32()-1.0;
            rsq=v1*v1+v2*v2;
        } while (rsq >= 1.0 || rsq==0.0);
        fac=sqrt(-2.0*log(rsq)/rsq);
        gset=v1*fac;
        iset=1;
        return v2*fac;
    } else {
        iset=0;
        return gset;
    }
}

// This generates unnormalized time domain noise with frequency spectrum given by "envelope".
// (Because it will be one-bitted immediately afterwards.)
void generate_time_noise_with_envelope(fftwf_complex *noise, double *envelope, int fftlen_noise, int fftlen_envelope, fftwf_plan fp_noise_inv, MtRand &mtr) {

    // double rnd1;
    // double rnd2;
    // double HALF_RAND_MAX = RAND_MAX/2.0;
    double multiplier;

    //  * generate fftlen_patch random cplx bit pairs (as doubles)
    /*
    for(int k=0; k<fftlen_noise; k++) {
      rnd1=static_cast<double>(rand());
      rnd1=rnd1 <= HALF_RAND_MAX ? -1 : 1;
      rnd2=static_cast<double>(rand());
      rnd2=rnd2 <= HALF_RAND_MAX ? -1 : 1;
      noise[k][0]=rnd1;
      noise[k][1]=rnd2;
    }*/
    //  * fourier transform them to get a Gaussian distribution
    //      average power per frequency bin is some constant P, and
    //      expected power in each bin is <P>.
    // fftwf_execute(fp_noise);
    // avg power per bin = 2 * fftlen_patch.
    //  * TODO: divide by sqrt(P), so avg power in each bin is 1.
    /* double sqrt_avg_power = sqrt((double) 2 * fftlen_noise);
    for(int k = 0; k<fftlen_noise; k++) {
      noise[k][0] /= sqrt_avg_power;
      noise[k][1] /= sqrt_avg_power;
    } */
    for (int k=0; k<fftlen_noise; k++) {
        noise[k][0] = gasdev(mtr);
        noise[k][1] = gasdev(mtr);
    }
    //  * multiply patch by sqrt(avp(nu)).
    //      expected power in bin nu is now avp(nu)
    for (int k = 0; k<fftlen_noise; k++) {
        // 0.5 is added so we get 0.5, ... fftlen_noise - 0.5, which is then rounded down.
        int index = (int) ( ((float)k + 0.5) / (float)fftlen_noise * (float)fftlen_envelope);
        multiplier = sqrt(envelope[ index ]);

        noise[k][0] *= multiplier;
        noise[k][1] *= multiplier;
        // if(k == 0)
        //  printf("noise[0][0] = %f, noise[0][1] = %f, noise power = %f, multiplier = %f, index = %d\n", noise[0][0], noise[0][1], noise[0][1] * noise[0][1] + noise[0][0] * noise[0][0], multiplier, index);
    }
    //  * inverse FFT
    fftwf_execute(fp_noise_inv);
}

// One-bits the data (called "noise") which has length fftlen_noise
// That is, values greater than 0 are set to 1, and less than 0 are
// set to -1.
void one_bit(fftwf_complex *noise, int fftlen_noise) {
    int sum_0_0 = 0;
    int sum_0_1 = 0;
    for (int k=0; k<fftlen_noise; k++) {
        noise[k][0] = (noise[k][0] < 0) ? -1 : 1;
        noise[k][1] = (noise[k][1] < 0) ? -1 : 1;
        sum_0_0 += noise[k][0];
        sum_0_1 += noise[k][1];
    }
    // printf("noise[0][0] = %f, noise[0][1] = %f\n", noise[0][0], noise[0][1]);
    // printf("sum_0_0 = %d, sum_0_1 = %d\n", sum_0_0, sum_0_1);
}

