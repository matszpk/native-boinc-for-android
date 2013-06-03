#include "fftw3.h"
#include "mtrand.h"
#include <cstdio>
#include <cmath>

#ifndef SETI_DR2FILTER_H
#define SETI_DR2FILTER_H

#if 0
// Filters for removing common waveforms from a channel.
template <typename T>
class waveform_filter {
    public:
        typedef void (* waveform_filter_funct)(std::vector<T> &channel, const
                std::vector<T> &waveform);
        waveform_filter(waveform_filter_funct F=NULL) : f(F) {};
        waveform_filter(const waveform_filter &F) : f(F.f) {};
        void operator ()(std::vector<T> &channel, const std::vector<T> &waveform) {
            f(channel,waveform);
        };
        operator bool() {
            return f!=0;
        };
        bool operator ==(const waveform_filter_funct &g) const {
            return (f==g);
        }
    private:
        waveform_filter_funct f;
};
#endif

// Filters for blanking indicated (via a blanking signal) bins of a channel
template <typename T>
class blanking_filter {
    public:
        typedef void (* blanking_filter_funct)(std::vector<T> &channel, const
                std::vector<int> &blanking_signal);
        blanking_filter(blanking_filter_funct F=NULL) : f(F) {};
        blanking_filter(const blanking_filter &F) : f(F.f) {};
        void operator ()(std::vector<T> &channel, const std::vector<int> &blanking_signal) {
            f(channel,blanking_signal);
        };
        operator bool() {
            return f!=0;
        };
        bool operator ==(const blanking_filter_funct &g) const {
            return (f==g);
        }
    private:
        blanking_filter_funct f;
};


template <typename T>
void randomize(std::vector<T> &channel, const std::vector<int> &blanking_signal) {
    size_t i;
    double rnd1;
    double rnd2;
    double HALF_RAND_MAX = RAND_MAX/2.0;
    for (i=0; i<channel.size(); i++) {
        if (blanking_signal[i]) {
            rnd1=static_cast<double>(rand());
            rnd1=rnd1 <= HALF_RAND_MAX ? -1 : 1;
            rnd2=static_cast<double>(rand());
            rnd2=rnd2 <= HALF_RAND_MAX ? -1 : 1;
            channel[i]=complex<double>(rnd1,rnd2);
        }
    }
}

double absdiff_percent(double *arr1, double *arr2, int len);

void generate_time_noise_with_envelope(fftwf_complex *noise, double *envelope, int fftlen_noise, int fftlen_envelope, fftwf_plan fp_noise_inv, MtRand &mtr);

void one_bit(fftwf_complex *noise, int fftlen_noise);

double gasdev(MtRand &mtr);

template <typename T>
void randomize_corrected(std::vector<T> &channel,
        const std::vector<int> &blanking_signal) {

    const int fftlen_envelope = 128;
    const int fftlen_patch = 512;
    MtRand mtr;
    mtr.srandom(-1);

    // Part 0:
    // * Set up ffts
    fftwf_complex *patch;
    fftwf_complex *envelope_part;
    fftwf_plan fp_envelope, fp_patch, fp_patch_inv; 
#if 0
    fftwf_plan fp_noise, fp_noise_inv;
#endif

    // * set aside fftlen_envelope doubles to be the power envelope in frequency space.
    double *envelope = new double[fftlen_envelope];
    patch = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex)*fftlen_patch);
    //   this will be used to FFT parts of the data to be added to envelope
    envelope_part = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex)*fftlen_envelope);

    fp_envelope = fftwf_plan_dft_1d(fftlen_envelope, envelope_part, envelope_part, FFTW_FORWARD, FFTW_MEASURE);
    fp_patch = fftwf_plan_dft_1d(fftlen_patch, patch, patch, FFTW_FORWARD, FFTW_MEASURE);
    fp_patch_inv = fftwf_plan_dft_1d(fftlen_patch, patch, patch, FFTW_BACKWARD, FFTW_MEASURE);

    // initialize the envelope array
    for (int i=0; i<fftlen_envelope; i++) {
        envelope[i] = 0;
    }


    // Part 1.a: find envelope (as it should be after one-bitting)

    // For counting to see if we've performed enough ffts
    // for a meaningful power envelope.
    int num_ffts_performed = 0;
    // * cycle through the #s in the channel, adding each of them to
    //   envelope_part
    //   env_index tells which envelope_part we are on
    for (unsigned int env_index=0;
            env_index<channel.size() / fftlen_envelope;
            env_index++) {
        bool filled_envelope_part = true;
        for (int i = 0; i < fftlen_envelope; i++) {
            unsigned int c = env_index * fftlen_envelope + i;
            // TODO: Remove this check for speed up?  It should not be necessary.
            if (c >= blanking_signal.size() || c >= channel.size()) {
                printf("Error in include/seti_dr2filter.h: randomize_corrected: channel index c out of bounds. %d %d %d\n",c,blanking_signal.size(),channel.size());
                fprintf(stderr, "Error in include/seti_dr2filter.h: randomize_corrected: channel index c out of bounds.%d %d %d\n",c,blanking_signal.size(),channel.size());
            }
            if (blanking_signal[c]) {
                filled_envelope_part = false;
                break;
            }
            envelope_part[i][0] = channel[c].real();
            envelope_part[i][1] = channel[c].imag();
        }
        //   * If you succeed in filling up the fft_arr without
        //   hitting any 0's in the blanking_signal: fft the fft_arr, square its absolute
        //   value, and add to the power arr.
        //   * If you don't fill up the fft_arr, discard it and start over.
        if (filled_envelope_part) {
            fftwf_execute(fp_envelope);
            for (int i = 0; i < fftlen_envelope; i++) {
                envelope[i] += envelope_part[i][0] * envelope_part[i][0]
                        + envelope_part[i][1] * envelope_part[i][1];
            }
            num_ffts_performed++;
        }
    }

    // * if you don't get very many envelope_part's, print an error msg and exit.
    //   (this means the blanking_signal had too many "holes" in it.)
    int min_ffts = 100;
    if (num_ffts_performed < min_ffts) {
        printf("Error in include/seti_dr2filter.h: randomize_corrected: num_ffts_performed < %d.  blanking_signal has too many holes?\n", min_ffts);
        fprintf(stderr, "Error in include/seti_dr2filter.h: randomize_corrected: num_ffts_performed < %d.  blanking_signal has too many holes?\n", min_ffts);
        exit(-1);
    }

    // * when you're done, normalize the power array:
    //    * add up the powers, and divide by fftlen_envelope
    //      to get the average power
    //    * divide the whole array by the average power
    //    * array is avp(nu), the average power at frequency nu, normalized
    //      so that <avp(nu)>_nu = 1.
    double total_power = 0;
    for (int i = 0; i < fftlen_envelope; i++) {
        total_power += envelope[i];
    }
    double avg_power = total_power / fftlen_envelope;
    for (int i = 0; i < fftlen_envelope; i++) {
        envelope[i] /= avg_power;
        // printf("init'ing envelope: envelope[%d] = %f\n", i, envelope[i]);
    }

    // Part 1.b: find envelope (as it should be before 1-bitting.)

    double *pre_envelope = new double[fftlen_envelope];
    double known_alpha = 0.547539;

#if 0
    If you uncomment this, be sure to uncomment the fftwf_free(noise) line below.

    fftwf_complex *noise;
    noise = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex)*fftlen_envelope);
    fp_noise = fftwf_plan_dft_1d(fftlen_envelope, noise, noise, FFTW_FORWARD, FFTW_MEASURE);
    fp_noise_inv = fftwf_plan_dft_1d(fftlen_envelope, noise, noise, FFTW_BACKWARD, FFTW_MEASURE);
    double *noise_power = new double[fftlen_envelope];
    double *noise_power_old = new double[fftlen_envelope];
    int N;
    int num_large_loop = 0;
    int num_small_loop = 0;

    for (int i = 0; i < fftlen_envelope; i++) // initialize pre_envelope, the final version of which will produce the main
        // envelope (once the pre_envelope spectrum has been turned into Gaussian noise, FFT^-1'd,
        // 1-bitted, and then FFT'd.)
    {
        pre_envelope[i] = envelope[i];
    }

    do {

        num_large_loop++;
        double count_0 = 0;
        double total_noise_0 = 0;

        generate_time_noise_with_envelope(noise, pre_envelope, fftlen_envelope, fftlen_envelope, fp_noise_inv, mtr);
        one_bit(noise, fftlen_envelope);
        fftwf_execute(fp_noise);
        for (int i = 0; i < fftlen_envelope; i++) {
            noise_power[i] = (noise[i][0] * noise[i][0] + noise[i][1] * noise[i][1]) / (2.0 * fftlen_envelope);
            noise_power_old[i] = (noise[i][0] * noise[i][0] + noise[i][1] * noise[i][1]) / (2.0 * fftlen_envelope);
        }
        // total_noise_0 += (noise[0][0] * noise[0][0] + noise[0][1] * noise[0][1]) / (2.0 * fftlen_envelope);
        // count_0++;
        // printf("avg_noise_0 = %f, pre_envelope[0] = %f\n", total_noise_0 / count_0, pre_envelope[0]);

        N = 1;

        do {
            pre_envelope[0] = 0.0001;
            num_small_loop++;

            for (int i = 0; i < fftlen_envelope; i++) { // avg = (<1 .. N> + <N + 1 .. 2N>) / 2
                noise_power_old[i] = (noise_power[i] + noise_power_old[i]) / 2.0;
                noise_power[i] = 0;  // initialize noise_power array to 0.
            }
            // printf("noise_power_old[0] = %f\n", noise_power_old[0]);
            for (int n = N; n < 2 * N; n++) {
                generate_time_noise_with_envelope(noise, pre_envelope, fftlen_envelope, fftlen_envelope, fp_noise_inv, mtr);
                // printf("after generate_time_noise: noise_power[0] = %f\n", (noise[0][0] * noise[0][0] + noise[0][1] * noise[0][1]) / (2.0 * fftlen_envelope));
                one_bit(noise, fftlen_envelope);
                // printf("after one_bit: noise_power[0] = %f\n", (noise[0][0] * noise[0][0] + noise[0][1] * noise[0][1]));
                fftwf_execute(fp_noise);
                for (int i = 0; i < fftlen_envelope; i++) {
                    noise_power[i] += (noise[i][0] * noise[i][0] + noise[i][1] * noise[i][1]) / (2.0 * fftlen_envelope);
                }
                // printf("after FFT^-1: noise_power[0] = %f\n", (noise[0][0] * noise[0][0] + noise[0][1] * noise[0][1]) / (2.0 * fftlen_envelope));
                // total_noise_0 += (noise[0][0] * noise[0][0] + noise[0][1] * noise[0][1]) / (2.0 * fftlen_envelope);
                // count_0++;
                // printf("avg_noise_0 = %f, pre_envelope[0] = %f\n", total_noise_0 / count_0, pre_envelope[0]);
            }
            for (int i = 0; i < fftlen_envelope; i++) {
                noise_power[i] /= N;
            }
            // Here's where you would do smoothing, if you wanted to.
            N *= 2;
        } while (absdiff_percent(noise_power, noise_power_old, fftlen_envelope) >= 0.001);

        for (int i = 0; i < fftlen_envelope; i++) {
            noise_power[i] = (noise_power[i] + noise_power_old[i]) / 2.0;
        }

        double test_total = 0;
        for (int i = 0; i < fftlen_envelope; i++) {
            test_total += noise_power[i];
        }
        printf("test_total = %f \n", test_total);
        printf("test_total / fftlen_envelope = %f (should be 1.0)\n", test_total / fftlen_envelope);

        double total_power = 0;
        double numerator = 0;
        double denominator = 0;
        double alpha;
        for (int i = 0; i < fftlen_envelope; i++) {
            numerator += std::abs(noise_power[i] - pre_envelope[i]);
            denominator += std::abs(1 - noise_power[i]);
            // printf("noise_power is: %f, pre_envelope is: %f\n", noise_power[i], pre_envelope[i]);
            // printf("numerator is: %f, denominator is: %f\n", numerator, denominator);

            // pre_envelope[i] = 2 * envelope[i] - (2 * noise_power[i] - pre_envelope[i]);
            // if(pre_envelope[i] < 0) pre_envelope[i] = 0;
        }
        alpha = numerator / denominator;
        printf("alpha is: %f\n", alpha);
        for (int i = 0; i < fftlen_envelope; i++) {
            pre_envelope[i] = (1 + alpha) * envelope[i] - alpha;
            if (pre_envelope[i] < 0) {
                pre_envelope[i] = 0;
            }
            total_power += pre_envelope[i];
        }
        if (total_power <= 0) {
            printf("Error in include/seti_dr2filter.h: randomize_corrected: total <= 0.\n");
            fprintf(stderr, "Error in include/seti_dr2filter.h: randomize_corrected: total <= 0.\n");
            exit(1);
        }
        for (int i = 0; i < fftlen_envelope; i++) { // set average power per bin to 1.
            pre_envelope[i] *= (fftlen_envelope / total_power);
        }
    } while (absdiff_percent(envelope, noise_power, fftlen_envelope) >= 0.005);

    printf("Finished part 1.b : large loop %d, small loop %d\n", num_large_loop, num_small_loop);
    fprintf(stderr, "Finished part 1.b : large loop %d, small loop %d\n", num_large_loop, num_small_loop);
    exit(1);

#endif

    double total_power3;
    for (int i = 0; i < fftlen_envelope; i++) {
        pre_envelope[i] = (1 + known_alpha) * envelope[i] - known_alpha;
        if (pre_envelope[i] < 0) {
            pre_envelope[i] = 0;
        }
        total_power3 += pre_envelope[i];
    }
    if (total_power3 <= 0) {
        printf("Error in include/seti_dr2filter.h: randomize_corrected: total <= 0.\n");
        fprintf(stderr, "Error in include/seti_dr2filter.h: randomize_corrected: total <= 0.\n");
        exit(1);
    }
    for (int i = 0; i < fftlen_envelope; i++) { // set average power per bin to 1.
        pre_envelope[i] *= (fftlen_envelope / total_power);
    }

    // Part 2:  You will need the pre_envelope from above

    // Some of the same variables as in randomize function
    // double rnd1;
    // double rnd2;
    // double HALF_RAND_MAX = RAND_MAX/2.0;

    int patch_index = 0;
    // double multiplier;
    // double total_power2;
    // double avg_power2;

    // * Next, cycle through the channel again.
    for (unsigned int j=0; j<channel.size(); j++) {
        // * When blanking_signal tells us to blank,
        if (blanking_signal[j]) { // start (or continue) blanking
            if (patch_index == 0) { // if it's time to start a new patch, then:
                generate_time_noise_with_envelope(patch, pre_envelope, fftlen_patch, fftlen_envelope,
                        fp_patch_inv, mtr);
                one_bit(patch, fftlen_patch);
            }
            //  * use bits from this distribution to fill the holes
            while (j < channel.size() && blanking_signal[j] == 1) { // short circuit the &&
                channel[j] = complex<double>( patch[patch_index][0],
                        patch[patch_index][1]);
                patch_index++;
                if (patch_index >= fftlen_patch) {
                    //  * when you don't have any patch bits left,
                    //    start a new patch, going back through the for loop.
                    // printf("temp message: j = %d, no patch bits left\n", j);
                    patch_index = 0;  // this line not necessary b/c duplicated below?
                    break;
                }
                j++;
            }
            patch_index = 0;  // start a new patch for each new blanking_signal "hole"
        } // end "if(blanking_signal[j])"
    } // end "for(int j=0; j<channel.size(); j++)"

    delete envelope;
    delete pre_envelope;

    fftwf_destroy_plan(fp_envelope);
    fftwf_destroy_plan(fp_patch);
    fftwf_destroy_plan(fp_patch_inv);
#if 0
    fftwf_destroy_plan(fp_noise);
    fftwf_destroy_plan(fp_noise_inv);
#endif

    fftwf_free(envelope_part);
    fftwf_free(patch);
    // fftwf_free(noise);
}

#if 0
template <typename T>
void direct_subtract(std::vector<T> &channel, const std::vector<T> &waveform) {
    size_t i;
    for (i=0; i<channel.size(); i++) {
        channel[i]-=waveform[i];
    }
}

template <typename T>
void scaled_subtract(std::vector<T> &channel, const std::vector<int> &waveform) {
    std::vector<complex<double> > a(least_squares(waveform,channel));
    size_t i;
    for (i=0; i<channel.size(); i++) {
        channel[i]-=(a[0]+complex<double>(a[1].real()*waveform[i].real(),a[1].imag()*waveform[i].imag()));
    }
}

template <typename T>
void smoothed_subtract(std::vector<T> &channel, const std::vector<T> &waveform) {
    std::vector<T> schannel(smooth(channel,50)),swaveform(smooth(waveform,50));
    std::vector<complex<double> > a(least_squares(swaveform,schannel));
    size_t i;
    for (i=0; i<channel.size(); i++) {
        channel[i]-=(a[0]+T(a[1].real()*waveform[i].real(),a[1].imag()*waveform[i].imag()));
    }
}

template <typename T>
void smoothed_subtract(std::vector<complex<T> > &channel, const
        std::vector<complex<T> > &waveform) {
    std::vector<complex<T> > schannel(smooth(channel,50)),swaveform(smooth(waveform,50));
    std::vector<complex<double> > a(least_squares(swaveform,schannel));
    size_t i;
    for (i=0; i<channel.size(); i++) {
        channel[i]-=complex<T>(
                a[0].real()+a[1].real()*waveform[i].real(),
                a[0].imag()+a[1].imag()*waveform[i].imag()
                );
    }
}


template <typename T>
void equalize_zeros_and_ones(std::vector<T> &channel, const std::vector<T>
        &waveform) {
    double max_wav;
    int i;
    for (i=0; i<waveform.size(); i++) {
        max_wav=std::max(max_wav,std::abs((double)waveform[i].real()+waveform[i].imag()));
    }
    for (i=0; i<channel.size(); i++) {
        if ((waveform[i].real()+waveform[i].imag())*channel[i].real() > 0) {
            // probability that this bit will change sign
            double pchange=std::abs(waveform[i].real()+waveform[i].imag())/(2*max_wav);
            double rnd=static_cast<double>(rand() & 0x3fff)/0x3fff;
            if (rnd < pchange) {
                channel[i]=T(-1*channel[i].real(),channel[i].imag());
            }
        }
        if ((waveform[i].real()+waveform[i].imag())*channel[i].imag() > 0) {
            // probability that this bit will change sign
            double pchange=std::abs(waveform[i].real()+waveform[i].imag())/(2*max_wav);
            double rnd=static_cast<double>(rand() & 0x3fff)/0x3fff;
            if (rnd < pchange) {
                channel[i]=T(channel[i].real(),-1*channel[i].imag());
            }
        }
    }
}

template <typename T>
void equalize_zeros_and_ones_c(std::vector<T> &channel, const std::vector<T>
        &waveform) {
    complex<double> max_wav;
    int i;
    for (i=0; i<waveform.size(); i++) {
        max_wav=complex<double>(
                std::max(max_wav.real(),std::abs((double)waveform[i].real())),
                std::max(max_wav.imag(),std::abs((double)waveform[i].imag()))
                );
    }
    for (i=0; i<channel.size(); i++) {
        if (waveform[i].real()*channel[i].real() > 0) {
            // probability that this bit will change sign
            double pchange=std::abs(waveform[i].real())/(2*max_wav.real());
            double rnd=static_cast<double>(rand() & 0x3fff)/0x3fff;
            if (rnd < pchange) {
                channel[i]=T(-1*channel[i].real(),channel[i].imag());
            }
        }
        if (waveform[i].imag()*channel[i].imag() > 0) {
            // probability that this bit will change sign
            double pchange=std::abs(waveform[i].imag())/(2*max_wav.imag());
            double rnd=static_cast<double>(rand() & 0x3fff)/0x3fff;
            if (rnd < pchange) {
                channel[i]=T(channel[i].real(),-1*channel[i].imag());
            }
        }
    }
}
#endif

#endif
