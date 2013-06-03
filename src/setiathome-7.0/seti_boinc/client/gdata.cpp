// Copyright 2003 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// In addition, as a special exception, the Regents of the University of
// California give permission to link the code of this program with libraries
// that provide specific optimized fast Fourier transform (FFT) functions and
// distribute a linked executable.  You must obey the GNU General Public 
// License in all respects for all of the code used other than the FFT library
// itself.  Any modification required to support these libraries must be
// distributed in source code form.  If you modify this file, you may extend 
// this exception to your version of the file, but you are not obligated to 
// do so. If you do not wish to do so, delete this exception statement from 
// your version.

#include "sah_config.h"
#include <algorithm>

#include "seti.h"

#include "gdata.h"

// in each case, see if the signal being copied is the same
// as what's already there, and return if so.
// This prevents the dirty flag from being set spuriously
//

void G_TRIPLET_INFO::copy(TRIPLET_INFO * ti, bool ib) {
    int i;
    if (peak_power==ti->t.peak_power && period==ti->t.period) return;
    peak_power = ti->t.peak_power;
    period = ti->t.period;
    tpotind0_0 = ti->tpotind0_0;
    tpotind0_1 = ti->tpotind0_1;
    tpotind1_0 = ti->tpotind1_0;
    tpotind1_1 = ti->tpotind1_1;
    tpotind2_0 = ti->tpotind2_0;
    tpotind2_1 = ti->tpotind2_1;
    for (i=0; i<TRIPLET_POT_LEN; i++) {
        pot_max[i] = ti->pot_max[i];
    }
    for (i=0; i<TRIPLET_POT_LEN; i++) {
        pot_min[i] = ti->pot_min[i];
    }
    dirty = true;
    is_best = ib;
}

void G_PULSE_INFO::copy(PULSE_INFO * pi, bool ib) {
    int i;
    if (score==pi->score && peak_power==pi->p.peak_power) return;
    peak_power = pi->p.peak_power;
    period = pi->p.period;
    score = pi->score;
    for (i=0; i<PULSE_POT_LEN; i++) {
        pot_max[i] = pi->pot_max[i];
    }
    //for (i=0; i<PULSE_POT_LEN; i++) {
    //    pot_min[i] = pi->pot_min[i];
    //}
    dirty = true;
    is_best = ib;
}

void G_GAUSS_INFO::copy(GAUSS_INFO * gi, bool ib) {
    size_t i;
    if (score==gi->score && chisqr==gi->g.chisqr) return;
    score = gi->score;
    peak_power = gi->g.peak_power;
    mean_power = gi->g.mean_power;
    chisqr = gi->g.chisqr;
    sigma = gi->g.sigma;
    fft_ind = gi->fft_ind;
    for (i=0; i<std::min(gi->g.pot.size(),static_cast<size_t>(GAUSS_POT_LEN)); i++) {
        pot[i] = gi->g.pot[i];
    }
    dirty = true;
    is_best = ib;
}

void G_SPIKE_INFO::copy(SPIKE_INFO * si, bool ib) {
}

void G_AUTOCORR_INFO::copy(AUTOCORR_INFO * si, bool ib) {
}
