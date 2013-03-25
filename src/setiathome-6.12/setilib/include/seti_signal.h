typedef enum {spike_t=1,gaussian_t=2,pulse_t=4,triplet_t=8, spike_small_t=16, gaussian_small_t=32, pulse_small_t=64, triplet_small_t=128, star_t=256} signal_types;
extern const int signal_type_cnt;
extern const char * signal_name[];
extern const int mp_type_cnt;
extern const char * mp_name[];

int          signal_index_by_type(int sigtype);
int          signal_type_by_index(int sigindex);
const char * signal_name_by_type(int sigtype);
const char * signal_name_by_index(int sigindex);

int signal_to_type(spike s);    
int signal_to_type(gaussian s); 
int signal_to_type(pulse s);    
int signal_to_type(triplet s);  
int signal_to_type(spike_small s);    
int signal_to_type(gaussian_small s); 
int signal_to_type(pulse_small s);    
int signal_to_type(triplet_small s);  
int signal_to_type(star s);    


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
    void populate(spike_small signal);
    void populate(gaussian_small signal);
    void populate(pulse_small signal);
    void populate(triplet_small signal);
    template<typename T> void populate_common(T signal);
    bool update_db();
};
