#include "setilib.h"

class RFI_ZONE : public rfi_zone {
    private:
        // bools to tell us which fields to check
        bool check_receiver_s4id;
        bool check_splitter_config;
        bool check_analysis_config;
        bool check_tape_id;
        bool check_workunit_id;
        bool check_result_id;
        bool check_time;
        bool check_baseband_freq;
        bool check_detection_freq;
        bool check_period;
        bool check_fft_len;
        bool check_signal_type;
        bool check_position;
        // functions to do the actual checking
        bool in_receiver_s4id_zone(const common_signal_t &s) const;
        bool in_splitter_config_zone(const common_signal_t &s) const;
        bool in_analysis_config_zone(const common_signal_t &s) const;
        bool in_tape_id_zone(const common_signal_t &s) const;
        bool in_workunit_id_zone(const common_signal_t &s) const;
        bool in_result_id_zone(const common_signal_t &s) const;
        bool in_time_zone(const common_signal_t &s) const;
        bool in_baseband_freq_zone(const common_signal_t &s) const;
        bool in_detection_freq_zone(const common_signal_t &s) const;
        bool in_period_zone(const common_signal_t &s) const;
        bool in_fft_len_zone(const common_signal_t &s) const;
        bool in_signal_type_zone(const common_signal_t &s) const;
        bool in_position_zone(const common_signal_t &s) const;
    public:
        RFI_ZONE() :
            rfi_zone(),
            check_receiver_s4id(false),
            check_splitter_config(false),
            check_analysis_config(false),
            check_tape_id(false),
            check_workunit_id(false),
            check_result_id(false),
            check_time(false),
            check_baseband_freq(false),
            check_detection_freq(false),
            check_period(false),
            check_fft_len(false),
            check_signal_type(false),
            check_position(false)
        {};
        RFI_ZONE(const RFI_ZONE &a) :
            rfi_zone(static_cast<const rfi_zone>(a)),
            check_receiver_s4id(a.check_receiver_s4id),
            check_splitter_config(a.check_splitter_config),
            check_analysis_config(a.check_analysis_config),
            check_tape_id(a.check_tape_id),
            check_workunit_id(a.check_workunit_id),
            check_result_id(a.check_result_id),
            check_time(a.check_time),
            check_baseband_freq(a.check_baseband_freq),
            check_detection_freq(a.check_detection_freq),
            check_period(a.check_period),
            check_fft_len(a.check_fft_len),
            check_signal_type(a.check_signal_type),
            check_position(a.check_position)
        {};
        bool is_in_zone(common_signal_t &signal) const;     // jeffc - const ?
        friend class rfi_zone_table;
};

class rfi_zone_table {
    private:
        static std::vector<RFI_ZONE> table;
        void read_rfi_zones();
        long nzones;
    public:
        typedef std::vector<RFI_ZONE>::iterator iterator;
        rfi_zone_table() {
            read_rfi_zones();
        };
        ~rfi_zone_table() {};
        iterator begin() {
            return table.begin();
        };
        iterator end() {
            return table.end();
        };
        size_t size() {
            return table.size();
        };
};

std::vector<RFI_ZONE> rfi_zone_table::table;

void rfi_zone_table::read_rfi_zones() {
    char query[64];

    RFI_ZONE zone;
    nzones=zone.count("");
    if (table.size() != nzones) {
        table.clear();
        table.reserve(nzones);
    }
    zone.open_query("","order by id","{+ALL_ROWS}");
    while (zone.get_next()) {
        // Determine which values in a signal need to be checked for this zone.
        if (!zone.max_receiver_s4id) {
            zone.max_receiver_s4id=INT32_MAX;
        }
        zone.check_receiver_s4id=(zone.min_receiver_s4id || (zone.max_receiver_s4id != INT32_MAX));
        if (zone.max_splitter_config==0) {
            zone.max_splitter_config=INT32_MAX;
        }
        zone.check_splitter_config=(zone.min_splitter_config || (zone.max_splitter_config != INT32_MAX));
        if (zone.max_analysis_config==0) {
            zone.max_analysis_config=INT32_MAX;
        }
        zone.check_analysis_config=(zone.min_analysis_config || (zone.max_analysis_config != INT32_MAX));
        if (zone.max_tape_id==0) {
            zone.max_tape_id=INT32_MAX;
        }
        zone.check_tape_id=(zone.min_tape_id || (zone.max_tape_id != INT32_MAX));
        if (zone.max_workunit_id==0) {
            zone.max_workunit_id=INT64_MAX;
        }
        zone.check_workunit_id=(zone.min_workunit_id || (zone.max_workunit_id != INT64_MAX));
        if (zone.max_result_id==0) {
            zone.max_result_id=INT64_MAX;
        }
        zone.check_result_id=(zone.min_result_id || (zone.max_result_id != INT64_MAX));
        if (zone.max_time==0) {
            zone.max_time=INFINITY;
        }
        zone.check_time=(zone.min_time != 0 || !isfinite(zone.max_time));
        zone.check_baseband_freq=isfinite(zone.central_baseband_freq) &&
                isfinite(zone.baseband_freq_width) &&
                (zone.baseband_freq_width>0.0);
        zone.check_detection_freq=isfinite(zone.central_detection_freq) &&
                isfinite(zone.detection_freq_width) &&
                (zone.detection_freq_width>0.0);
        zone.check_period=isfinite(zone.central_period) && isfinite(zone.period_width) &&
                (zone.central_period != 0) and (zone.period_width != 0);
        zone.check_fft_len=(zone.fft_len_flags != 0);
        zone.check_signal_type=(zone.signal_type_flags != 0);
        zone.check_position=(zone.angular_distance != 0) && isfinite(zone.angular_distance);
        table.push_back(zone);
    }
    zone.close_query();
}

// most of these rely on short circuit evaluation
bool RFI_ZONE::in_receiver_s4id_zone(const common_signal_t &s) const {
    if (!check_receiver_s4id) {
        return true;
    }
    // DB functions return false on error.  How should we handle that, throw()?
    if (s.result_p->wuid->id == 0) {
        s.result_p->cached_fetch();
    }
    if (s.result_p->wuid->group_info->id == 0) {
        s.result_p->wuid->cached_fetch();
    }
    if (s.result_p->wuid->group_info->receiver_cfg->id == 0) {
        s.result_p->wuid->group_info->cached_fetch();
    }
    if (s.result_p->wuid->group_info->receiver_cfg->s4_id == 0) {
        s.result_p->wuid->group_info->receiver_cfg->cached_fetch();
    }
    int s4_id=s.result_p->wuid->group_info->receiver_cfg->s4_id;
    return (s4_id >= min_receiver_s4id) && (s4_id <= max_receiver_s4id);
}

bool RFI_ZONE::in_splitter_config_zone(const common_signal_t &s) const {
    if (!check_splitter_config) {
        return true;
    }
    // DB functions return false on error.  How should we handle that, throw()?
    if (s.result_p->wuid->id == 0) {
        s.result_p->cached_fetch();
    }
    if (s.result_p->wuid->group_info->id == 0) {
        s.result_p->wuid->cached_fetch();
    }
    if (s.result_p->wuid->group_info->splitter_cfg->id == 0) {
        s.result_p->wuid->group_info->cached_fetch();
    }
    int scfg=s.result_p->wuid->group_info->splitter_cfg->id;
    return (scfg >= min_splitter_config) && (scfg <= max_splitter_config);
}

bool RFI_ZONE::in_analysis_config_zone(const common_signal_t &s) const {
    // this checks the value of the "base" or "parent" analysis config rather
    // than the derrived analysis config.
    if (!check_analysis_config) {
        return true;
    }
    // DB functions return false on error.  How should we handle that, throw()?
    if (s.result_p->wuid->id == 0) {
        s.result_p->cached_fetch();
    }
    if (s.result_p->wuid->group_info->id == 0) {
        s.result_p->wuid->cached_fetch();
    }
    if (s.result_p->wuid->group_info->analysis_cfg->id == 0) {
        s.result_p->wuid->group_info->cached_fetch();
    }
    if (s.result_p->wuid->group_info->analysis_cfg->keyuniq == 0) {
        s.result_p->wuid->group_info->analysis_cfg->cached_fetch();
    }
    int parent_cfg=s.result_p->wuid->group_info->analysis_cfg->keyuniq/1024;
    return (parent_cfg>=min_analysis_config) && (parent_cfg<=max_analysis_config);
}

bool RFI_ZONE::in_tape_id_zone(const common_signal_t &s) const {
    if (!check_tape_id) {
        return true;
    }
    // DB functions return false on error.  How should we handle that, throw()?
    if (s.result_p->wuid->id == 0) {
        s.result_p->cached_fetch();
    }
    if (s.result_p->wuid->group_info->id == 0) {
        s.result_p->wuid->cached_fetch();
    }
    //if (s.result_p->wuid->group_info->tape_info == 0) {
    if (s.result_p->wuid->group_info->tape_info->id == 0) {
        s.result_p->wuid->group_info->cached_fetch();
    }
    int tapeid=s.result_p->wuid->group_info->tape_info->id;
    return (tapeid>=min_tape_id) && (tapeid<=max_tape_id);
}

bool RFI_ZONE::in_workunit_id_zone(const common_signal_t &s) const {
    if (!check_tape_id) {
        return true;
    }
    // DB functions return false on error.  How should we handle that, throw()?
    if (s.result_p->wuid->id == 0) {
        s.result_p->cached_fetch();
    }
    int wuid=s.result_p->wuid->id;
    return (wuid>=min_workunit_id) && (wuid<=max_workunit_id);
}

bool RFI_ZONE::in_result_id_zone(const common_signal_t &s) const {
    if (!check_result_id) {
        return true;
    }
    return (s.result_p->id >= min_result_id) && (s.result_p->id <= max_result_id);
}

bool RFI_ZONE::in_time_zone(const common_signal_t &s) const {
    return (!check_time) || ((s.time>=min_time) && (s.time<=max_time));
}
bool RFI_ZONE::in_baseband_freq_zone(const common_signal_t &s) const {
    // TBD
    if (!check_baseband_freq) {
        return true;
    }
    if (s.result_p->wuid->id == 0) {
        s.result_p->cached_fetch();
    }
    if (s.result_p->wuid->group_info->id == 0) {
        s.result_p->wuid->cached_fetch();
    }
    return false;
}

bool RFI_ZONE::in_detection_freq_zone(const common_signal_t &s) const {
    bool retval;
    if (!check_detection_freq) {
        return true;
    }
    // a signal has frequency width, so we need to check if any
    // part of that frequency crosses the zone.
    double upzone=central_detection_freq+0.5*detection_freq_width;
    double lozone=central_detection_freq-0.5*detection_freq_width;
    double upsig=s.detection_freq+0.5*s.subband_sample_rate/s.fft_len;
    double losig=s.detection_freq-0.5*s.subband_sample_rate/s.fft_len;
//fprintf(stderr, "lozone = %lf\n", lozone);
//fprintf(stderr, "upzone = %lf\n", upzone);
//fprintf(stderr, "losig  = %lf\n", losig);
//fprintf(stderr, "upsig  = %lf\n", upsig);
    retval =  ((upsig>=lozone) && (upsig<=upzone)) || // upper part of singal in lower part of zone
            ((losig<=upzone) && (losig>=lozone)) || // lower part of signal in upper part of zone
            ((losig>=lozone) && (upsig<=upzone));   // signal entirely within zone
//fprintf(stderr, "dfz retval = %d\n", retval);
//fprintf(stderr, "=======================\n", upsig);
    return retval;
}

bool RFI_ZONE::in_period_zone(const common_signal_t &s) const {
    if (!check_period) {
        return true;
    }
    double upzone=central_period+0.5*period_width;
    double lozone=central_period-0.5*period_width;
    return (s.period>=lozone) && (s.period<=upzone);
}

bool RFI_ZONE::in_fft_len_zone(const common_signal_t &s) const {
    return (!check_fft_len) || ((s.fft_len & fft_len_flags) != 0);
}

bool RFI_ZONE::in_signal_type_zone(const common_signal_t &s) const {
    return (!check_signal_type) || ((s.sigtype & signal_type_flags) != 0);
}

bool RFI_ZONE::in_position_zone(const common_signal_t &s) const {
    if (!check_position) {
        return true;
    }
    double angdist=co_SkyAngle(ra, dec, s.ra, s.decl);
    return (angdist <= angular_distance);
}

bool RFI_ZONE::is_in_zone(common_signal_t &s) const {
// Each of these "anded" routines behave in the following way:
// If there is no zone specification for that field, return true.
// If there is a specification for that field and the signal is within the
//   bounds of that specification, return true.
// If there is a specification for that field and the signal is not within the
//   bounds of that specification, return false.
// Thus, we move through this series of routines until we encounter
// a specification that the signal is not within the bounds of, in which
// case is_in_zone() returns false.  Otherwise, is_in_zone() returns
// true.  Careful - a completely null zone will result in every signal
// getting marked as rfi!
    return in_receiver_s4id_zone(s)       &&
            in_splitter_config_zone(s)     &&
            in_analysis_config_zone(s)     &&
            in_tape_id_zone(s)             &&
            in_workunit_id_zone(s)         &&
            in_result_id_zone(s)           &&
            in_time_zone(s)                &&
            in_baseband_freq_zone(s)       &&
            in_detection_freq_zone(s)      &&
            in_period_zone(s)              &&
            in_fft_len_zone(s)             &&
            in_signal_type_zone(s)         &&
            in_position_zone(s);
}

//static rfi_zone_table zone_table;   // this being here caused a seg fault

bool rfi_zone(common_signal_t &signal, std::vector<spike> &spike_field) {

    bool retval;
    int zone_counter=0;

    static rfi_zone_table zone_table;

    //std::vector<RFI_ZONE> rfi_zone_table::table;
    rfi_zone_table::iterator zone;

    // Iterate through the table until we get to the end or find out the signal
    // is in a zone.  This is a possible location for optimization.  We could
    // either create index maps for the table on the various parameters, or we
    // could just sort the table to put the zones that catch the most signals
    // first.  For now we'll just check every signal against every zone.

    for (zone=zone_table.begin(), retval=false; zone != zone_table.end() ; zone++ ) {
        zone_counter++;
        if (zone->is_in_zone(signal)) {
            retval = true;
            //fprintf(stderr, "Found RFI via zone %d : signal type %d id %ld time %lf\n",
            //        zone_counter, signal.sigtype, signal.sigid, signal.time);
            break;
        }
    }

    return retval;
}
