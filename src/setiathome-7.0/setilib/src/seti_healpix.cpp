#include <stdio.h>
#include <math.h>
#include "arr.h"

#include "setilib.h"

#ifndef BOOLEAN
#define BOOLEAN unsigned char
#define TRUE 1
#define FALSE 0
#endif

static const double D2R=(M_PI/180.0);  // decimal degrees to radians
const long nside    = 2048;       // number of cubic pixels
const long fpix_res = 10;         // Hz

const unsigned long center_freq = 1420000000;
const unsigned long low_freq    = 1418750000;
const unsigned long high_freq   = 1421250000;
const unsigned long low_fpix    = low_freq  / fpix_res;
const unsigned long high_fpix   = high_freq / fpix_res;

Healpix_Base sky(2048, NEST, SET_NSIDE);  // a pixelated sky - the NSIDE of 2048 should come from the sci config.


//___________________________________________________________________________________
long npix2qpix(long long npix) {
//___________________________________________________________________________________
    return(long)(npix >> 32);
}

//___________________________________________________________________________________
void npix2qpix(std::vector<long long> &npix, std::vector<long> &qpix) {
//___________________________________________________________________________________

    unsigned long i;
    for (i=0; i < npix.size(); i++) {
        qpix.push_back(npix2qpix(npix[i]));
    }

}

//___________________________________________________________________________________
long npix2fpix(long long npix) {
//___________________________________________________________________________________
    return(long)(npix & 0x00000000ffffffff);
}

//___________________________________________________________________________________
void npix2fpix(std::vector<long long> &npix, std::vector<long> &fpix) {
//___________________________________________________________________________________

    unsigned long i;
    for (i=0; i < npix.size(); i++) {
        fpix.push_back(npix2fpix(npix[i]));
    }

}

//___________________________________________________________________________________
long long co_radeclfreq2npix(double ra, double decl, double freq) {
//___________________________________________________________________________________

    long qpix;
    long long npix;

    qpix = co_radecl2qpix(ra, decl);
    npix = qpix;
    npix = (npix << 32) + (unsigned long)round(freq / fpix_res);
    return npix;
}

//___________________________________________________________________________________
void co_npix2radeclfreq(double &ra, double &decl, double &freq, long long npix) {
//___________________________________________________________________________________

    long qpix;

    freq = (npix & 0x00000000ffffffffLLU) * fpix_res;
    qpix = npix >> 32;
    co_qpix2radecl(ra, decl, qpix);
}


//___________________________________________________________________________________
long co_radecl2qpix(double ra, double decl) {
//___________________________________________________________________________________

    double theta, phi;
    long qpix;

    theta = (90.0 - decl) * ((double)M_PI/180.0);
    phi   = (ra * 15.0)   * ((double)M_PI/180.0);
    ang2pix_nest(nside, theta, phi, &qpix);

    return(qpix);
}

//___________________________________________________________________________________
void co_qpix2radecl(double &ra, double &decl, long qpix) {
//___________________________________________________________________________________

    double theta, phi;

    pix2ang_nest(nside, qpix, &theta, &phi);
    decl = 90.0 - (theta / ((double)M_PI/180.0));
    ra   = (phi / ((double)M_PI/180.0)) / 15.0;
}

//___________________________________________________________________________________
void get_disc(double ra, double decl, double radius, std::vector<int> &listpix) {
//___________________________________________________________________________________

    double theta, phi;

    //fprintf(stderr, "ra decl\n");
    theta = D2R * (90.0 - decl);
    phi   = D2R * (ra * 15.0);

    pointing coord(theta, phi);

    sky.query_disc (coord, D2R*radius, listpix);
}

//___________________________________________________________________________________
void get_disc(long qpix, double radius, std::vector<int> &listpix) {
//___________________________________________________________________________________

    double ra, decl;

    //fprintf(stderr, "qpix\n");
    co_qpix2radecl(ra, decl, qpix);

    get_disc(ra, decl, radius, listpix);
}

//___________________________________________________________________________________
void get_disc(long long npix, double radius, std::vector<int> &listpix) {
//___________________________________________________________________________________

    long qpix = (long)(npix >> 32);

    //fprintf(stderr, "npix\n");
    //fprintf(stderr, "qpix = %ld\n", qpix);

    get_disc(qpix, radius, listpix);
}


//___________________________________________________________________________________
void get_neighbors(long qpix, long my_neighbors[8]) {
//___________________________________________________________________________________

    int i;
    fix_arr< int, 8 > n;

    sky.neighbors(qpix, n);

    for (i=0; i<8; i++) {
        my_neighbors[i] = n[i];
    }
}

//___________________________________________________________________________________
long get_neighbor(long qpix, int position) {
//___________________________________________________________________________________

    long neighbors[8];

    get_neighbors(qpix, neighbors);

    return(neighbors[position]);
}


//___________________________________________________________________________________
void turn_qpixes_hot(std::list<long>& qpixlist, int &hotpix_update_count,
        int &hotpix_insert_count, bool high_priority=false) {
//___________________________________________________________________________________

    std::list<long>::iterator qpix_i;
    hotpix hotpix;
    time_t last_hit_time;

    if (high_priority) {
        last_hit_time = 1;
    } else {
        time(&last_hit_time);
    }

    for (qpix_i = qpixlist.begin(); qpix_i != qpixlist.end(); qpix_i++) {
        if (hotpix.fetch(*qpix_i)) {
            if (hotpix.last_hit_time == 1) {
                continue;    // no change if already high priority
            }
            hotpix.last_hit_time = last_hit_time;
            hotpix.update();
            hotpix_update_count++;
        } else {
            hotpix.id = *qpix_i;
            hotpix.last_hit_time = last_hit_time;
            hotpix.insert(*qpix_i);
            hotpix_insert_count++;
        }
    }
}

//___________________________________________________________________________________
void qpix_set_hotpix(std::list<long>& qpixlist, int &hotpix_update_count,
        int &hotpix_insert_count, bool high_priority) {
//___________________________________________________________________________________
// Takes a list of qpixes and adds to this list all qpixes within radius of each
// original qpix.  The resulting "super list" is then uniqued and each remaining
// qpix is turned hot in the hotpix table.  Thus, each original qpix and radius
// surrounding qpixes are turned hot.

    int i;
    const int num_neighbors = 8;
    long neighbors[num_neighbors];
    std::list<long>::iterator qpix_i;
    std::list<long> neighborlist;

    for (qpix_i = qpixlist.begin(); qpix_i != qpixlist.end(); qpix_i++) {
        get_neighbors(*qpix_i, neighbors);
        for (i=0; i<num_neighbors; i++) {
            neighborlist.push_back(neighbors[i]);
        }
    }
    for (qpix_i = neighborlist.begin(); qpix_i != neighborlist.end(); qpix_i++) {
        qpixlist.push_back(*qpix_i);
    }
    qpixlist.sort();
    qpixlist.unique();
    turn_qpixes_hot(qpixlist, hotpix_update_count, hotpix_insert_count, high_priority);
}

//___________________________________________________________________________________
void qpix_set_hotpix(long center_qpix, bool high_priority=false) {
//___________________________________________________________________________________

    std::list<long> qpixlist;

    // dummy counters
    int hotpix_update_count=0;
    int hotpix_insert_count=0;

    qpixlist.push_back(center_qpix);
    // the following call will also set all pixels hot within radius
    qpix_set_hotpix(qpixlist, hotpix_update_count, hotpix_insert_count, high_priority);
}

//___________________________________________________________________________________
long signals_by_disc(spike signal, std::vector<spike>& signals, std::vector<int>& disc,
        long long min_id=0, bool filter_rfi=true) {
//___________________________________________________________________________________
    char *d_string = "{+index(spike spike_qpix)}";
    return(signals_by_disc(signal, signals, disc, min_id, filter_rfi, d_string));
}

//___________________________________________________________________________________
long signals_by_disc(autocorr signal, std::vector<autocorr>& signals, std::vector<int>& disc,
        long long min_id=0, bool filter_rfi=true) {
//___________________________________________________________________________________
    char *d_string = "{+index(autocorr autocorr_qpix)}";
    return(signals_by_disc(signal, signals, disc, min_id, filter_rfi, d_string));
}

//___________________________________________________________________________________
long signals_by_disc(gaussian signal, std::vector<gaussian>& signals, std::vector<int>& disc,
        long long min_id=0, bool filter_rfi=true) {
//___________________________________________________________________________________
    char *d_string = "{+index(gaussian gaussian_qpix)}";
    return(signals_by_disc(signal, signals, disc, min_id, filter_rfi, d_string));
}

//___________________________________________________________________________________
long signals_by_disc(triplet signal, std::vector<triplet>& signals, std::vector<int>& disc,
        long long min_id=0, bool filter_rfi=true) {
//___________________________________________________________________________________
    char *d_string = "{+index(triplet triplet_qpix)}";
    return(signals_by_disc(signal, signals, disc, min_id, filter_rfi, d_string));
}

//___________________________________________________________________________________
long signals_by_disc(pulse signal, std::vector<pulse>& signals, std::vector<int>& disc,
        long long min_id=0, bool filter_rfi=true) {
//___________________________________________________________________________________
    char *d_string = "{+index(pulse pulse_qpix)}";
    return(signals_by_disc(signal, signals, disc, min_id, filter_rfi, d_string));
}


//___________________________________________________________________________________
bool stars_by_disc(star &my_star, std::vector<star>& stars, std::vector<int>& disc) {
//___________________________________________________________________________________

    unsigned int i;
    bool retval;
    // double ra, decl;
    char c_string[256];
    char *d_string = "{+index(star star_qpix)}";
    std::string where_clause;
    std::string directive_clause;

    stars.clear();

    for (i=0; i < disc.size(); i++) {
        sprintf(c_string, "where qpix == %d order by qpix", disc[i]);
        where_clause = c_string;
        directive_clause = d_string;

        retval = my_star.open_query(where_clause,"",directive_clause);
        if (retval) {
            while ((retval = my_star.get_next())) {
                stars.push_back(my_star);
            }
            retval = my_star.close_query();
        }
    }
    return (retval);
}

