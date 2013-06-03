#ifndef SETI_SIGNAL_H
#define SETI_SIGNAL_H

typedef enum {spike_t=1,gaussian_t=2,pulse_t=4,triplet_t=8, spike_small_t=16, gaussian_small_t=32, pulse_small_t=64, triplet_small_t=128, star_t=256, autocorr_t=512, autocorr_small_t=1024} signal_types;
extern const int signal_type_cnt;
extern const char *signal_name[];
extern const int mp_type_cnt;
extern const char *mp_name[];

int          signal_index_by_type(int sigtype);
int          signal_type_by_index(int sigindex);
const char *signal_name_by_type(int sigtype);
const char *signal_name_by_index(int sigindex);

int signal_to_type(spike s);
int signal_to_type(gaussian s);
int signal_to_type(pulse s);
int signal_to_type(triplet s);
int signal_to_type(autocorr s);
int signal_to_type(spike_small s);
int signal_to_type(gaussian_small s);
int signal_to_type(pulse_small s);
int signal_to_type(triplet_small s);
int signal_to_type(autocorr_small s);
int signal_to_type(star s);

template<typename T>
void flip_frequency(T &signal) {
//#define DEBUG_FLIP_SIGNAL_FREQUENCY

    double center_freq=1420000000, new_freq, new_detection_freq, new_barycentric_freq, new_chirp_rate;

    // what about those very rare instances where the full band ceneter is not 1420MHz?
    // See fix_flip.cpp
    new_freq = center_freq + (center_freq-signal.freq);
    new_detection_freq = center_freq + (center_freq-signal.detection_freq);
    // new bary freq has the same offset (both magnitude and direction) relationship to 
    // new detection freq as did old bary freq to old detection freq 
    new_barycentric_freq = new_detection_freq + (signal.barycentric_freq - signal.detection_freq);  
    new_chirp_rate = -signal.chirp_rate;
    if (new_chirp_rate == -0) new_chirp_rate = 0;

#ifdef DEBUG_FLIP_SIGNAL_FREQUENCY
    fprintf(stderr, "                   old                       new\n");
    fprintf(stderr, "freq               %lf         %lf         %f\n", signal.freq, new_freq, 
                                                                                   ((signal.freq-1420000000)*2)-(signal.freq-new_freq)); 
    fprintf(stderr, "detection freq     %lf         %lf         %f\n", signal.detection_freq, new_detection_freq, 
                                                                                   ((signal.detection_freq-1420000000)*2)-(signal.detection_freq-new_detection_freq)); 
    fprintf(stderr, "barycentric freq   %lf         %lf         %f\n", signal.barycentric_freq, new_barycentric_freq, 
                                                                       (signal.barycentric_freq-signal.detection_freq) - (new_barycentric_freq-new_detection_freq)); 
    fprintf(stderr, "chirp rate         %lf                   %lf              %f\n", signal.chirp_rate, new_chirp_rate, fabs(signal.chirp_rate)-fabs(new_chirp_rate)); 
#endif

    signal.freq             = new_freq; 
    signal.detection_freq   = new_detection_freq;
    signal.barycentric_freq = new_barycentric_freq;
    signal.chirp_rate       = new_chirp_rate;

    return;
} 

template<typename T>
int get_next_signal(T &signal) {
//#define FLIP_SIGNAL_FREQUENCY

    int get_next_retval;

    get_next_retval = signal.get_next();

//fprintf(stderr, "reserved = %d\n", signal.reserved);
    // do any signal pre-processing and/or correction here
#ifdef FLIP_SIGNAL_FREQUENCY
    if(signal.reserved == 0) {
        flip_frequency(signal);
    }
#endif

    return(get_next_retval);
}    


class common_signal_t {
    public:
        signal_types sigtype;
        sqlint8_t sigid;         // original signal ID
        result *result_p;
        double  peak_power;
        double  mean_power;
        double  time;
        double  ra;
        double  decl;
        double  freq;
        double  period;
        double  delay;
        double  detection_freq;
        double  barycentric_freq;
        double  baseband_freq;
        long    fft_len;
        double  subband_sample_rate;
        double  chirp_rate;
        long    rfi_checked;
        long    rfi_found;

        void populate(spike signal);
        void populate(gaussian signal);
        void populate(pulse signal);
        void populate(triplet signal);
        void populate(autocorr signal);
        void populate(spike_small signal);
        void populate(gaussian_small signal);
        void populate(pulse_small signal);
        void populate(triplet_small signal);
        void populate(autocorr_small signal);
        template<typename T> void populate_common(T signal);
        bool update_db();
};
#endif
