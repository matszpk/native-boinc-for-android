// fakedata [ options ] filename
//
// Title      : fakedata.c
// Programmer : Jeff Cobb / David Anderson
// History    : 12/13/95 - first release
//            : 10/25/05 - Modified for setiathome_enhanced Eric Korpela
// Copyright (c) 1998 University of California Regents

// Credits for seti@home reference package:  
// algorithm design: Dan Werthimer (UCB),  
//                   Mike Lampton, (UCB),  
//                   Charles Donnelly (UCB),  
//                   Jeff Cobb (UCB)  
// reference code programming: Jeff Cobb  
// seti@home network design and programming: David Anderson

// This program is to be used to generate fake digital data
// for testing seti@home algorithms.  It generates random
// noise embedded with any of the following signal types:
// - continuous
// - pulsed
// - chirped
// - showing an gaussian power profile over time
// The randomness of the noise can be colored by:
// - change in noise amplitude over time
// - favoring certain frequencies (changing noise
//   amplitude over frequency)

// The signal to noise ratio you want to use to emulate a setiathome work
// unit depends upon the signal type and the FFT length you want to find it at.
// For example if you want to build a gaussian that is 5 times the noise power
// at an FFT length of 16384 you should specify a noise amplitude of 1 and
// a gaussian amplitude of 5/sqrt(16384) (about 0.04).  If you want to specify
// a spike of 30 times the mean noise power at an FFT length of 128k you would
// specify a sine wave of amplitude 30/sqrt(128k) (about 0.083).

// The output file contains
// the number of data points and an array of floating
// point numbers representing power over time.
// That the array is a series of complex values,
// using 2 array elements for each data point.
// Real parts are held in even elements (starting with [0])
// and imaginary parts are held in the adjacent odd elements.

// options:
//
// -sr n
//	samples per second
// -dur x
//	duration of data (seconds)
// -sine amp freq chirp start end
//	inject a sine wave, amplitude amp
//	freq: frequency at center of time interval
//	(note: freqs must lie in [-sr/2, sr/2]
//	chirp rate in Hz/sec
//	NOTE: chirp is applied starting from time 0
//	(not necessarily from the start of the sine wave)
//	Start and end times; if zero, whole duration
// -noise amp
//	inject noise of amplitude amp;
// -gauss amp midpt sig freq chirp
//	Inject a Gaussian signal with the given amplitude and midpoint,
//	halfpower width (seconds), frequency (Hz), chirp rate (Hz/sec)
// -pulse period duty
//      Inject a pulse signal with the given period (seconds) and duty cycle (percent).  It
//      is "encased" in the Gaussian envelope determined by the -gaussian parameters
// -ascii
//	generate ascii data
//	default: 1-bit output samples (binary).

//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "analyze.h"
#include "seti_header.h"
#include "xml_util.h"

#define NEG_LN_ONE_HALF     0.693
#if 0
#define EXP(a,b,c)          exp(-(NEG_LN_ONE_HALF * pow((double)(a) - (double)(b), 2.0)) / (double)(c))
#endif

struct SIGNAL_DESC {
    double sample_rate;
    double dur;
    bool onebit;
    double noise_amp;
    double sine_amp;
    double sine_freq;
    double sine_chirp;
    double sine_start;
    double sine_end;
    double gauss_amp;
    double gauss_midpt;
    double gauss_sigma;
    double gauss_freq;
    double gauss_chirp;
    double pulse_period;
    double pulse_duty_cycle;
};

void bad_args() {
    fprintf(stderr, "bad args\n");
    exit(1);
}

float randu() {
// Uniform random numbers between 0 and 1
  static bool first=true;
  if (first) {
    srand(time(0));
    first=false;
  }
  return static_cast<float>(rand())/RAND_MAX;
}

float rande() {
// exponentially distributed random numbers
  return -log(randu());
}

float randn() {
// normally distributed random numbers
  static float last=0;
  float a,b,h=0;
  if (last==0) {
    while (h==0 || h>=1) {
      a=2*randu()-1;
      b=2*randu()-1;
      h=a*a+b*b;
    }
    h=sqrt(-2*log(h)/h);
    last=a*h;
    return b*h;
  } else {
    a=last;
    last=0;
    return a;
  }
}



void getopts(int argc, char* argv[], SIGNAL_DESC& sd) {
    int i;
    sd.onebit = true;
    for (i=1; i<argc; i++) {
	if (!strcmp(argv[i],"-sr")) {
	    if (sscanf(argv[++i],"%lf", &sd.sample_rate) != 1) bad_args();
	} else if (strcmp(argv[i],"-dur") == 0) {
	    if (sscanf(argv[++i],"%lf", &sd.dur) != 1) bad_args();
	} else if (strcmp(argv[i],"-noise") == 0) {
	    if (sscanf(argv[++i],"%lf", &sd.noise_amp) != 1) bad_args();
	} else if (strcmp(argv[i],"-sine") == 0) {
	    if (sscanf(argv[++i],"%lf", &sd.sine_amp) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.sine_freq) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.sine_chirp) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.sine_start) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.sine_end) != 1) bad_args();
	} else if (strcmp(argv[i],"-gauss") == 0) {
	    if (sscanf(argv[++i],"%lf", &sd.gauss_amp) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.gauss_midpt) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.gauss_sigma) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.gauss_freq) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.gauss_chirp) != 1) bad_args();
	} else if( strcmp(argv[i],"-pulse") == 0) {
	    if (sscanf(argv[++i],"%lf", &sd.pulse_period) != 1) bad_args();
	    if (sscanf(argv[++i],"%lf", &sd.pulse_duty_cycle) != 1) bad_args();

	} else if (!strcmp(argv[i], "-ascii")) {
	    sd.onebit = false;
	} else {
	    bad_args();
	}
    }
}

// the following depends on how the splitter works

int freq_to_wu(SIGNAL_DESC& sd, float freq) {
    return cnvt_hz_bin(freq, sd.sample_rate, 256);
}

int main (int argc, char** argv) {
    SIGNAL_DESC sd;
    int npoints, cnt, i, j;
    long idum;
    unsigned int c;
    double real, imag, mytime;
    workunit_header wu;
    std::vector<unsigned char> data;

    time_t st=time(0);


    if (argc < 2) bad_args();

    idum = -time(NULL);

    memset(&sd, 0, sizeof(sd));
    getopts(argc, argv, sd);

// some default values --  added by EK /////////////////////////////
    if (sd.sample_rate == 0) sd.sample_rate=9765.625;
    if (sd.noise_amp == 0) sd.noise_amp=1;
    if (sd.dur==0) {
      npoints=1024*1024;
      sd.dur=npoints/sd.sample_rate;
    } else {
      npoints = (int)(sd.sample_rate * sd.dur);
    }
    data.reserve(npoints/4);
    if (sd.gauss_sigma == 0) sd.gauss_sigma=12.5;
////////////////////////////////////////////////////////////////////    

    if (sd.sine_end == 0) sd.sine_end = sd.dur;

// Added workunit header - EK ///////////////////////////////////
    sprintf(wu.name,"fake.%d.0",st);
///////////////////////// Group Info ////////////////////////////
    sprintf(wu.group_info->name,"fake.%d",st);
///////////////////////// Tape Info /////////////////////////////
    sprintf(wu.group_info->tape_info->name,"fake");
    wu.group_info->tape_info->start_time=time_t_to_jd(st);
    wu.group_info->tape_info->last_block_time=time_t_to_jd(st);
///////////////////////// Recv Conf /////////////////////////////
    wu.group_info->receiver_cfg->s4_id=1;
    sprintf(wu.group_info->receiver_cfg->name,"fake");
    wu.group_info->receiver_cfg->beam_width=5.0/60.0;
    wu.group_info->receiver_cfg->center_freq=1420.0;
    wu.group_info->receiver_cfg->az_corr_coeff.push_back(0.0);
    wu.group_info->receiver_cfg->zen_corr_coeff.push_back(0.0);
///////////////////////// Recorder Cfg //////////////////////////
    sprintf(wu.group_info->recorder_cfg->name,"fake");
    wu.group_info->recorder_cfg->bits_per_sample=2;
    wu.group_info->recorder_cfg->beams=1;
    wu.group_info->recorder_cfg->sample_rate=sd.sample_rate;
///////////////////////// Splitter Cfg //////////////////////////
    wu.group_info->splitter_cfg->version=0.17;
    sprintf(wu.group_info->splitter_cfg->data_type,"encoded");
    sprintf(wu.group_info->splitter_cfg->filter,"fft");
    sprintf(wu.group_info->splitter_cfg->window,"welsh");
    wu.group_info->splitter_cfg->fft_len=1;
    wu.group_info->splitter_cfg->ifft_len=1;
///////////////////////// Data Desc /////////////////////////////
    wu.group_info->data_desc.start_ra=0;
    wu.group_info->data_desc.start_dec=18.4;
    wu.group_info->data_desc.end_dec=18.4;
    wu.group_info->data_desc.time_recorded_jd=time_t_to_jd(st);
    sprintf(wu.group_info->data_desc.time_recorded,"%s",jd_string(time_t_to_jd(st)));
    wu.group_info->data_desc.nsamples=npoints;
    wu.group_info->data_desc.true_angle_range=sd.dur/sd.gauss_sigma*wu.group_info->receiver_cfg->beam_width;
    {
      time_t dt=0;
      double dr=wu.group_info->data_desc.true_angle_range/(cos(18.4*M_PI/180)*15*sd.dur);
      while (dt<(sd.dur+5)) {
        coordinate_t tmp;
        tmp.time=time_t_to_jd(st+dt);
        tmp.dec=18.4;
        tmp.ra=dt*dr;
        wu.group_info->data_desc.coords.push_back(tmp);
        dt+=5;
      }
    }
///////////////////////// Subband Desc /////////////////////////////
    wu.subband_desc.number=0;
    wu.subband_desc.center=1420000000;
    wu.subband_desc.base=1420000000;
    wu.subband_desc.sample_rate=sd.sample_rate;
    

///////////////////////// Analysis Cfg /////////////////////////////
    wu.group_info->analysis_cfg->spike_thresh=24.0;
    wu.group_info->analysis_cfg->spikes_per_spectrum=1;
    wu.group_info->analysis_cfg->gauss_null_chi_sq_thresh=2.2249999;
    wu.group_info->analysis_cfg->gauss_chi_sq_thresh=1.41999996;
    wu.group_info->analysis_cfg->gauss_power_thresh=3;
    wu.group_info->analysis_cfg->gauss_peak_power_thresh=3.2;
    wu.group_info->analysis_cfg->gauss_pot_length=64;
    wu.group_info->analysis_cfg->pulse_thresh=19.5;
    wu.group_info->analysis_cfg->pulse_display_thresh=0.5;
    wu.group_info->analysis_cfg->pulse_max=40960;
    wu.group_info->analysis_cfg->pulse_min=16;
    wu.group_info->analysis_cfg->pulse_fft_max=8192;
    wu.group_info->analysis_cfg->pulse_pot_length=256;
    wu.group_info->analysis_cfg->triplet_thresh=8.5;
    wu.group_info->analysis_cfg->triplet_max=131072;
    wu.group_info->analysis_cfg->triplet_min=16;
    wu.group_info->analysis_cfg->triplet_pot_length=256;
    wu.group_info->analysis_cfg->pot_overlap_factor=0.5;
    wu.group_info->analysis_cfg->pot_t_offset=1;
    wu.group_info->analysis_cfg->pot_min_slew=0.00209999993;
    wu.group_info->analysis_cfg->pot_max_slew=0.0104999999;
    wu.group_info->analysis_cfg->chirp_resolution=0.333;
    wu.group_info->analysis_cfg->analysis_fft_lengths=262136;
    wu.group_info->analysis_cfg->bsmooth_boxcar_length=8192;
    wu.group_info->analysis_cfg->bsmooth_chunk_size=32768;
    {
      chirp_parameter_t tmp;
      tmp.chirp_limit=20;
      tmp.fft_len_flags=262136;
      wu.group_info->analysis_cfg->chirps.push_back(tmp);
      tmp.chirp_limit=50;
      tmp.fft_len_flags=65528;
      wu.group_info->analysis_cfg->chirps.push_back(tmp);
    }
    wu.group_info->analysis_cfg->pulse_beams=1;
    wu.group_info->analysis_cfg->max_signals=3000;
    wu.group_info->analysis_cfg->max_spikes=3000;
    wu.group_info->analysis_cfg->max_gaussians=3000;
    wu.group_info->analysis_cfg->max_pulses=3000;
    wu.group_info->analysis_cfg->max_triplets=3000;






/////////////////////////////////////////////////////////////////    

    fprintf (stderr, "sample rate: %f\n", sd.sample_rate);
    fprintf (stderr, "duration: %f\n", sd.dur);
    fprintf (stderr, "number of points: %d\n", npoints);
    fprintf (stderr, "noise: %f\n", sd.noise_amp);
    fprintf (stderr, "sine_amp: %f\n", sd.sine_amp);
    fprintf (stderr, "sine_freq: %f (wu %d)\n", sd.sine_freq,
	freq_to_wu(sd, sd.sine_freq)
    );
    fprintf (stderr, "sine_chirp: %f\n", sd.sine_chirp);
    fprintf (stderr, "sine_start: %f\n", sd.sine_start);
    fprintf (stderr, "sine_end: %f\n", sd.sine_end);
    fprintf (stderr, "gauss_amp: %f\n", sd.gauss_amp);
    fprintf (stderr, "gauss_midpt: %f\n", sd.gauss_midpt);
    fprintf (stderr, "gauss_sigma: %f\n", sd.gauss_sigma);
    fprintf (stderr, "gauss_freq: %f (wu %d)\n", sd.gauss_freq,
	freq_to_wu(sd, sd.gauss_freq)
    );
    fprintf (stderr, "gauss_chirp: %f\n", sd.gauss_chirp);
    fprintf (stderr, "pulse_period: %f\n", sd.pulse_period);
    fprintf (stderr, "pulse_duty_cycle: %f\n", sd.pulse_duty_cycle);

    fprintf (stderr, "data format: %s\n", sd.onebit?"binary":"ascii");

    cnt = 0;
    if(!sd.onebit) {
	printf("nsamples=%d\n", npoints);
	printf("real   imag\n");
    }
    double sine_phase = 0, sine_freq;
    double sine_midpt = (sd.sine_end - sd.sine_start)/2;
    double gauss_phase = 0, gauss_freq;
    double dt = sd.dur/npoints;
    double sigma_sq = sd.gauss_sigma*sd.gauss_sigma;
    double pulseActive = 1.0;
    double pulseTime = sd.pulse_period*sd.pulse_duty_cycle/100.0;

    for (i=0; i<npoints; i++) {
	real = imag = 0;
	mytime = (sd.dur*i)/npoints;
	double ratio = i/(double)npoints;
	pulseActive = 1.0;

	if (sd.noise_amp > 0) {
	    real += randn()*sd.noise_amp;
	    imag += randn()*sd.noise_amp;
	}
	if (sd.sine_amp) {
	    if (mytime >= sd.sine_start && mytime <= sd.sine_end) {
		real += sd.sine_amp*cos(sine_phase);
		imag -= sd.sine_amp*sin(sine_phase);
	    }
	    sine_freq = sd.sine_freq + sd.sine_chirp*mytime;
	    sine_phase += M_PI*2*dt*sine_freq;
	}
	if (sd.pulse_period > 0) {
            if( (mytime - (floor(mytime/sd.pulse_period))*sd.pulse_period) > pulseTime ) {
		pulseActive = 0.0;
            }
        }
	if (sd.gauss_amp > 0) {
	    double gauss_factor = EXP(mytime, sd.gauss_midpt, sigma_sq);
	    real += gauss_factor*pulseActive*sd.gauss_amp*cos(gauss_phase);
	    imag -= gauss_factor*pulseActive*sd.gauss_amp*sin(gauss_phase);
	    gauss_freq = sd.gauss_freq + sd.gauss_chirp*mytime;
	    gauss_phase += M_PI*2*dt*gauss_freq;
	}

	if (sd.onebit) {
	    c >>= 2;
	    if (real >= 0) c |= 0x8000;
	    if (imag >= 0) c |= 0x4000;
	    cnt += 2;
	    if (cnt == 16) {
		cnt = 0;
		data.push_back(c/256);
		data.push_back(c&255);
	    }
	} else {
	    printf("%e %e\n", real, imag);
	}
    }
    // flush out remaining samples
    if (sd.onebit) {
	for (i=0; i<7; i++) {
	    c >>= 2;
	    cnt += 2;
	    if (cnt == 16) {
		cnt = 0;
		data.push_back(c/256);
		data.push_back(c&255);
	    }
	}
	printf("<workunit>\n");
	std::cout << wu;
	std::string tmpstr=xml_encode_string(data,_x_setiathome);
	printf("<data length=%ld encoding=\"%s\">",tmpstr.size(),
            xml_encoding_names[_x_setiathome]);
	fwrite(tmpstr.c_str(),tmpstr.size(),1,stdout);
	printf("</data>\n</workunit>\n");
    }
}

void print_error( int e ) {
    printf( "Error %d\n", e );
    exit(1);
}

