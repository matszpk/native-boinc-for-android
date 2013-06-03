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

#ifndef SAH_GFX_H
#define SAH_GFX_H

#include "gdata.h"
#include "reduce.h"
#include "gutil.h"
#include "sah_gfx_base.h"


// SAH_GRAPHICS combines all the info needed to draw S@h graphics:
// - user preferences (GRAPHICS_PREFS)
// - generic project graphics (SAH_GRAPHICS_BASE)
// - data from the analysis (GDATA)
// - internal state
// Its member functions do the rendering
//
class SAH_GRAPHICS: public SAH_GRAPHICS_BASE {
    PROGRESS inner_progress;
    PROGRESS outer_progress;
    PROGRESS_2D outer_progress_2d;        // outer progress for 2D display
	RIBBON_GRAPH rnd_graph;
    RIBBON_GRAPH sin_graph;
    MOVING_TEXT_PANEL text_panels[3];
    void render_pillars(double, double);
    void render_panels(double, double);
    void render_headsup(double, double);
    void init_text_panels();
    void get_data_info_string(char*);
	void get_user_info_string(char*);
    void get_analysis_info_string(char*);
    int choose_signal_to_display(double time_of_day);
public:
    void render(int x, int y, double t);
    void data_struct_init();
    void graphics_thread_init();
    void reread_prefs();
    void setup_given_prefs();
};

extern SAH_GRAPHICS sah_graphics;

#endif

