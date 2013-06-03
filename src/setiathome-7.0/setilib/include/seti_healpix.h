// forward declarations for app specific items
class star;                 // app setiathome
// end forward declarations

long npix2qpix(long long npix);
long npix2fpix(long long npix);
long long co_radeclfreq2npix(double ra, double decl, double freq);
void co_npix2radeclfreq(double &ra, double &decl, double &freq, long long npix);
long co_radecl2qpix(double ra, double decl);
void co_qpix2radecl(double &ra, double &decl, long pix);
void get_disc(double ra, double decl, double radius, std::vector<int> &listpix);
void get_disc(long qpix, double radius, std::vector<int> &listpix);
void get_disc(long long npix, double radius, std::vector<int> &listpix);
long signals_by_disc(spike signal,    std::vector<spike>& signals,    std::vector<int>& disc, long long min_id, bool filter_rfi);
long signals_by_disc(autocorr signal, std::vector<autocorr>& signals, std::vector<int>& disc, long long min_id, bool filter_rfi);
long signals_by_disc(gaussian signal, std::vector<gaussian>& signals, std::vector<int>& disc, long long min_id, bool filter_rfi);
long signals_by_disc(triplet signal,  std::vector<triplet>& signals,  std::vector<int>& disc, long long min_id, bool filter_rfi);
long signals_by_disc(pulse signal,    std::vector<pulse>& signals,    std::vector<int>& disc, long long min_id, bool filter_rfi);
bool stars_by_disc(star &my_star, std::vector<star>& stars, std::vector<int>& disc);
void get_neighbors(long qpix, long neighbors[8]);
long get_neighbor(long qpix, int position);
void qpix_set_hotpix(std::list<long>& qpixlist, int &hotpix_update_count, int &hotpix_insert_count, bool high_priority=false);
void qpix_set_hotpix(long qpix, bool high_priority);
int turn_qpix_hot(long qpix, bool high_priority=false);

extern const unsigned long low_fpix;
extern const unsigned long high_fpix;

enum {SW_QPIX, W_QPIX, NW_QPIX, N_QPIX, NE_QPIX, E_QPIX, SE_QPIX, S_QPIX};

template<typename T>
long signals_by_disc(T &signal, std::vector<T>& signals, std::vector<int>& disc, long long min_id=0, bool filter_rfi=true, char *d_string="") {

    int i;
    long raw_count=0;
    bool retval;
    //double ra, decl;
    long long lower_npix, upper_npix;
    char c_string[256];
    std::string where_clause;
    std::string directive_clause;

    signals.clear();

    for (i=0; i < disc.size(); i++) {
        //co_qpix2radecl(ra, decl, disc[i]);

        // disregard frequency by including the entire possible frequency range
        lower_npix = ((long long)disc[i] << 32);
        upper_npix = ((long long)disc[i] << 32) + 0xffffffff;

        sprintf(c_string, "where q_pix >= %lld and q_pix <= %lld", lower_npix, upper_npix);
        where_clause = c_string;
        directive_clause = d_string;

        retval = signal.open_query(where_clause, "",directive_clause);
        if (retval) {
            //while (retval = signal.get_next()) {
            while (retval = get_next_signal(signal)) {
                raw_count++;
                if (signal.id >= min_id) {
                    if (!filter_rfi || signal.rfi_found == 0) {
                        signals.push_back(signal);
                    }
                }
            }
            retval = signal.close_query();
        }
    }

    //return (retval);
    return (raw_count);
}

template<typename T>
sqlint8_t count_signals_by_disc(T &signal, std::vector<int>& disc) {

    int i;
    long count=0;
    long long lower_npix, upper_npix;
    char c_string[256];
    std::string where_clause;

    for (i=0; i < disc.size(); i++) {

        // disregard frequency by including the entire possible frequency range
        lower_npix = ((long long)disc[i] << 32);
        upper_npix = ((long long)disc[i] << 32) + 0xffffffff;

        // the order by clause is to force the use of the q_pix index.
        sprintf(c_string, "where q_pix >= %lld and q_pix <= %lld",  lower_npix, upper_npix);
        where_clause = c_string;

        count += signal.count(where_clause);
    }

    return (count);
}

template<typename T>
int trim_disc(std::vector<T>& signals, long disc_center, double disc_radius) {

    int i, out_of_disc_count;
    double disc_ra, disc_decl, signal_radius;
    std::vector<T> indisc_signals;

    co_qpix2radecl(disc_ra, disc_decl, disc_center);


    for (i=0, out_of_disc_count=0; i <  signals.size(); i++) {
        signal_radius = co_SkyAngle(signals[i].ra, signals[i].decl,
                disc_ra, disc_decl);
        if (signal_radius <= disc_radius) {
            indisc_signals.push_back(signals[i]);
        } else {
            out_of_disc_count++;
        }
    }

    signals.swap(indisc_signals);

    return(out_of_disc_count);
}
