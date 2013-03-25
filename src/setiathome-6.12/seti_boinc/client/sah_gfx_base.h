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

#include <vector>
#include "boinc_api.h"

#include "gutil.h"
#include "gdata.h"
#include "graphics_data.h"

#include "reduce.h"

typedef enum {
    TEXT_STYLE_PILLARS,    // panels delimited by cylinders
    TEXT_STYLE_PANELS,    // rotating panels
    TEXT_STYLE_HEADSUP      // 2D text in corners
} TEXT_STYLE;

// User's graphics-related per-project preferences for SETI@home
//
struct GRAPHICS_PREFS {
    TEXT_STYLE text_style;
    GRAPH_STYLE graph_style;
    double max_fps;                 // max frames per second
    double max_cpu;                 // max pct of CPU
    double graph_alpha;
    double pitch_period;
    double pitch_range;     // degrees
    double roll_period;
    double roll_range;
    int starfield_size;
    double starfield_speed;
    double start_hue, hue_change;   // determine graph colors
    double grow_time;
    double hold_time;

    void parse_project_prefs(char*);
    void defaults();
	GRAPHICS_PREFS() { defaults(); };
};

class SAH_GRAPHICS_BASE: /* public GDATA, */ public GRAPHICS_PREFS {
public:
    double roll_angle;
    double roll_phase;
    double pitch_angle;
    double pitch_phase;
    STARFIELD starfield;
    TEXTURE_DESC seti_logo_texture;
    TEXTURE_DESC user_logo_texture;	
    TEXTURE_DESC background_texture;
    APP_INIT_DATA app_init_data;
    double viewpoint_distance;
    int width, height;
    REDUCED_ARRAY_RENDER rarray;
    SAH_SHMEM* sah_shmem;
    GDATA* gdata;

    SAH_GRAPHICS_BASE();
    void data_struct_init();
    void graphics_thread_init();
    void reread_prefs();
    void setup_given_prefs();

    void resize(int, int);
    void start_rotate();
    void incr_rotate(double dt);
    void end_rotate();
    void draw_pillars();
	void render_logos();
    void render_background();
    void render_3d_graph(double time_of_day);
};

extern void set_viewpoint(double);
extern bool nographics();
