
// Copyright 2007 Regents of the University of California

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

#include "sah_gfx_main.h"
#include "boinc_api.h"
#include "graphics2.h"

extern double progress;
SAH_SHMEM* sah_shmem;
REDUCED_ARRAY_GEN rarray;
GDATA* sah_graphics;
bool nographics_flag = false;

void update_shmem() {
    if (sah_graphics->countdown > 0) {
        sah_graphics->countdown--;
    }
    boinc_wu_cpu_time(sah_graphics->cpu_time);
    sah_graphics->progress=progress;
}

void sah_graphics_init(APP_INIT_DATA& app_init_data) {
    if (nographics_flag) return;
    if ((app_init_data.host_info.m_nbytes != 0)  &&
        (app_init_data.host_info.m_nbytes <= (double)(48*1024*1024))
    ) {
        fprintf(stderr,"Low memory machine... Disabling graphics.\n");
        fprintf(stderr,"%f <= %f\n",app_init_data.host_info.m_nbytes,(double)48*1024*1024);
        nographics_flag = true;
        return;
    }
    sah_shmem = (SAH_SHMEM*)boinc_graphics_make_shmem("setiathome", sizeof(SAH_SHMEM));
    if (!sah_shmem) {
        sah_shmem=(SAH_SHMEM*)boinc_graphics_get_shmem("setiathome");
    }
    if (!sah_shmem) {
        nographics_flag = true;
        return;
    }
    sah_graphics = &(sah_shmem->gdata);
    boinc_register_timer_callback(update_shmem);
}
