class algorithm_t {
    public:
        bool run;
        long mask;
        bool need_spike_field;
        bool (*function)(common_signal_t &signal, std::vector<spike> &spike_field);
        char name[64];
};

int load_algorithm_vector(std::vector<algorithm_t> &algorithms);
