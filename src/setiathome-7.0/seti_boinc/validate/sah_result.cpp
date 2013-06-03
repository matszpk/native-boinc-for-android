#include <stdio.h>
#include <math.h>

#include "parse.h"
#include "sah_result.h"

// the difference between two numbers,
// as a fraction of the largest in absolute value
//
double rel_diff(double x1, double x2) {
    if (x1 == 0 && x2 == 0) return 0;
    if (x1 == 0) return 1;
    if (x2 == 0) return 1;

    double d1 = fabs(x1);
    double d2 = fabs(x2);
    double d = fabs(x1-x2);
    if (d1 > d2) return d/d1;
    return d/d2;
}

double abs_diff(double x1, double x2) {
    return fabs(x1-x2);
}


// return true if the two signals are the same
// within numerical tolerances
//
bool SIGNAL::roughly_equal(SIGNAL& s) {
    double second = 1.0/86400.0;    // 1 second as a fraction of a day

    if (type != s.type) return false;

    // tolerances common to all signals
    if (abs_diff(ra, s.ra)          > .00066) return false;  // .01 deg
    if (abs_diff(decl, s.decl)            > .01) return false;     // .01 deg
    if (abs_diff(time, s.time)          > second) return false;  // 1 sec
    if (abs_diff(freq, s.freq)          > .01) return false;     // .01 Hz
    if (abs_diff(chirp_rate, s.chirp_rate)  > .01) return false;     // .01 Hz/s
    if (fft_len != s.fft_len) return false;                  // equal

    switch (type) {
        case SIGNAL_TYPE_SPIKE:
        case SIGNAL_TYPE_BEST_SPIKE:
            if (rel_diff(power, s.power)        > .01) return false;  // 1%
            return true;
        case SIGNAL_TYPE_GAUSSIAN:
        case SIGNAL_TYPE_BEST_GAUSSIAN:
            if (rel_diff(peak_power, s.peak_power)      > .01) return false;  // 1%
            if (rel_diff(mean_power, s.mean_power)      > .01) return false;  // 1%
            if (rel_diff(sigma, s.sigma)        > .01) return false;  // 1%
            if (rel_diff(chisqr, s.chisqr)      > .01) return false;  // 1%
            //if (rel_diff(max_power, s.max_power)  > .01) return false;  // ?? jeffc
            return true;
        case SIGNAL_TYPE_PULSE:
        case SIGNAL_TYPE_BEST_PULSE:
            if (rel_diff(power, s.power)        > .01) return false;  // 1%
            if (rel_diff(mean_power, s.mean_power)      > .01) return false;  // 1%
            if (abs_diff(period, s.period)      > .01) return false;  // .01 sec
            if (rel_diff(snr, s.snr)        > .01) return false;  // 1%
            if (rel_diff(thresh, s.thresh)      > .01) return false;  // 1%
            return true;
        case SIGNAL_TYPE_TRIPLET:
        case SIGNAL_TYPE_BEST_TRIPLET:
            if (rel_diff(power, s.power)        > .01) return false;  // 1%
            if (rel_diff(mean_power, s.mean_power)      > .01) return false;  // 1%
            if (rel_diff(period, s.period)      > .01) return false;  // 1%
            return true;
	case SIGNAL_TYPE_AUTOCORR:
	case SIGNAL_TYPE_BEST_AUTOCORR:
	    if (rel_diff(power, s.power)        > .01) return false; // 1%
            if (rel_diff(delay, s.delay)      > .01) return false;  // 1%
	    return true;
    }
    return false;
}

// parse a SETI@home result file
//
int SAH_RESULT::parse_file(FILE* f) {
    char buf[1024];
    SIGNAL s;
    double d;
    int i;

    num_signals = 0;

    memset(&s, 0, sizeof(s));
    while (fgets(buf, 256, f)) {

        if (match_tag(buf, "<spike>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_SPIKE;
            num_signals++;

        } else if (match_tag(buf, "<best_spike>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_BEST_SPIKE;

        } else if (match_tag(buf, "<autocorr>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_AUTOCORR;
            num_signals++;

        } else if (match_tag(buf, "<best_autocorr>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_BEST_AUTOCORR;

        } else if (match_tag(buf, "<gaussian>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_GAUSSIAN;
            num_signals++;

        } else if (match_tag(buf, "<best_gaussian>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_BEST_GAUSSIAN;

        } else if (match_tag(buf, "<pulse>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_PULSE;
            num_signals++;

        } else if (match_tag(buf, "<best_pulse>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_BEST_PULSE;

        } else if (match_tag(buf, "<triplet>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_TRIPLET;
            num_signals++;

        } else if (match_tag(buf, "<best_triplet>")) {
            memset(&s, 0, sizeof(s));
            s.type = SIGNAL_TYPE_BEST_TRIPLET;

        } else if (parse_double(buf, "<ra>", d)) {
            s.ra = d;
        } else if (parse_double(buf, "<decl>", d)) {
            s.decl = d;
        } else if (parse_double(buf, "<power>", d)) {
            s.power = d;
        } else if (parse_double(buf, "<time>", d)) {
            s.time = d;
        } else if (parse_double(buf, "<delay>", d)) {
            s.delay = d;
        } else if (parse_double(buf, "<peak_power>", d)) {
            s.peak_power = d;
        } else if (parse_double(buf, "<mean_power>", d)) {
            s.mean_power = d;
        } else if (parse_double(buf, "<max_power>", d)) {
            s.max_power = d;
        } else if (parse_double(buf, "<sigma>", d)) {
            s.sigma = d;
        } else if (parse_double(buf, "<chisqr>", d)) {
            s.chisqr = d;
        } else if (parse_double(buf, "<period>", d)) {
            s.period = d;
        } else if (parse_double(buf, "<snr>", d)) {
            s.snr = d;
        } else if (parse_double(buf, "<thresh>", d)) {
            s.thresh = d;
        } else if (parse_double(buf, "<freq>", d)) {
            s.freq = d;
        } else if (parse_int(buf, "<fft_len>", i)) {
            s.fft_len = i;
        
	} else if (match_tag(buf, "</spike>")) {
            if (s.type != SIGNAL_TYPE_SPIKE) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</best_spike>")) {
            if (s.type != SIGNAL_TYPE_BEST_SPIKE) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</autocorr>")) {
            if (s.type != SIGNAL_TYPE_AUTOCORR) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</best_autocorr>")) {
            if (s.type != SIGNAL_TYPE_BEST_AUTOCORR) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</gaussian>")) {
            if (s.type != SIGNAL_TYPE_GAUSSIAN) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</best_gaussian>")) {
            if (s.type != SIGNAL_TYPE_BEST_GAUSSIAN) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</pulse>")) {
            if (s.type != SIGNAL_TYPE_PULSE) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</best_pulse>")) {
            if (s.type != SIGNAL_TYPE_BEST_PULSE) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</triplet>")) {
            if (s.type != SIGNAL_TYPE_TRIPLET) {
                return -1;
            }
            signals.push_back(s);

        } else if (match_tag(buf, "</best_triplet>")) {
            if (s.type != SIGNAL_TYPE_BEST_TRIPLET) {
                return -1;
            }
            signals.push_back(s);

        }
    }

    return 0;
}

// return true if the given signal is roughly equal to a signal
// from the result
//
bool SAH_RESULT::has_roughly_equal_signal(SIGNAL& s) {
    unsigned int i;
    for (i=0; i<signals.size(); i++) {
        if (signals[i].roughly_equal(s)) return true;
    }
    return false;
}

// return true if each signal from each result is roughly equal
// to a signal from the other result
//
bool SAH_RESULT::strongly_similar(SAH_RESULT& s) {
    unsigned int i;
    for (i=0; i<s.signals.size(); i++) {
        s.signals[i].checked = false;
    }
    for (i=0; i<signals.size(); i++) {
        if (!s.has_roughly_equal_signal(signals[i])) return false;
    }
    for (i=0; i<s.signals.size(); i++) {
        if (s.signals[i].checked) continue;
        if (!has_roughly_equal_signal(s.signals[i])) return false;
    }
    return true;
}

// return true if at least half the signals (and at least one)
// from each result are roughly equal to a signal from the other result
//
bool SAH_RESULT::weakly_similar(SAH_RESULT& s) {
    unsigned int n1, n2;
    unsigned int m1, m2;
    unsigned int i;

    n1 = signals.size();
    n2 = s.signals.size();

    m1 = 0;
    for (i=0; i<signals.size(); i++) {
        if (s.has_roughly_equal_signal(signals[i])) {
            m1++;
        }
    }
    m2 = 0;
    for (i=0; i<s.signals.size(); i++) {
        if (has_roughly_equal_signal(s.signals[i])) {
            m2++;
        }
    }
    if (m1 == 0) return false;
    if (m2 == 0) return false;
    if (m1 < (n1+1)/2) return false;
    if (m2 < (n2+1)/2) return false;
    return true;
}

bool SAH_RESULT::bad_values() {

    return false;
}
