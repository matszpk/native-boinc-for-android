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

#ifdef _WIN32
#include "boinc_win.h"
#include <gl/gl.h>           // Header File For The OpenGL32 Library
#include <gl/glu.h>          // Header File For The GLu32 Library
#if !defined(__MINGW32__) && !defined(_MSC_VER)
      #include <gl/glaux.h>        // Header File For The Glaux Library
#endif
#include <glut/glut.h>
#endif

#ifndef _WIN32
#include <cmath>
#include <ctime>

#ifdef __APPLE_CC__
#include <Carbon/Carbon.h>
#include <DrawSprocket/DrawSprocket.h>
#include <AGL/agl.h>
#include <AGL/aglRenderers.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#endif
#endif

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "s_util.h"
#include "db/db_table.h"
#include "db/schema_master.h"
#include "boinc_gl.h"
#include "parse.h"
#include "graphics2.h"
#include "sah_gfx_base.h"
#include "gutil.h"
#include "reduce.h"
#include "graphics_data.h"
#include "filesys.h"

#define PI2 (2*3.1415926)

#define SETI_LOGO_FILENAME "seti_logo"
#define USER_LOGO_FILENAME "user_logo"
#define BACKGROUND_FILENAME "background"

// maximum resolutions for power graph
#define MAX_GRAPH_RES_X 200
#define MAX_GRAPH_RES_Y 100

extern bool mouse_down;

using namespace std;

// The following must agree with the strings in
// project_specific_prefs.inc
//

void GRAPHICS_PREFS::parse_project_prefs(char *buf) {
	std::string text;

    if (!buf) return;
    if (parse_str(buf, "<text_style>", text)) {
        if (!strcmp(text.c_str(), "Pillars")) text_style = TEXT_STYLE_PILLARS;
        if (!strcmp(text.c_str(), "Panels")) text_style = TEXT_STYLE_PANELS;
        if (!strcmp(text.c_str(), "Heads-up")) text_style = TEXT_STYLE_HEADSUP;
    }

    if (parse_str(buf, "<graph_style>", text)) {
        if (!strcmp(text.c_str(), "Rectangles")) graph_style = GRAPH_STYLE_RECTANGLES;
        if (!strcmp(text.c_str(), "Wireframe")) graph_style = GRAPH_STYLE_WIREFRAME;
        if (!strcmp(text.c_str(), "Surface")) graph_style = GRAPH_STYLE_SURFACE;
        if (!strcmp(text.c_str(), "Planes")) graph_style = GRAPH_STYLE_PLANES;
    }
    parse_double(buf, "<max_fps>", max_fps);
    parse_double(buf, "<max_cpu>", max_cpu);
    parse_double(buf, "<grow_time>", grow_time);
    parse_double(buf, "<hold_time>", hold_time);
    parse_double(buf, "<graph_alpha>", graph_alpha);
    parse_double(buf, "<roll_period>", roll_period);
    parse_double(buf, "<roll_range>", roll_range);
    parse_double(buf, "<pitch_period>", pitch_period);
    parse_double(buf, "<pitch_range>", pitch_range);
    parse_int(buf, "<starfield_size>", starfield_size);
    parse_double(buf, "<starfield_speed>", starfield_speed);
    parse_double(buf, "<start_hue>", start_hue);
    parse_double(buf, "<hue_change>", hue_change);
}

void GRAPHICS_PREFS::defaults() {
    text_style = TEXT_STYLE_PILLARS;
    graph_style = GRAPH_STYLE_RECTANGLES;
    max_fps = 20;
    max_cpu = 30;
    grow_time = 3;
    hold_time = 2;
    graph_alpha = 0.7;

    pitch_period = 10;
    pitch_range = 20;
    roll_period = 30;
    roll_range = 30;
	starfield_size = 2000;
	starfield_speed = 400.0f;
    
    start_hue = 0.2;
    hue_change = 0.7;
}

SAH_GRAPHICS_BASE::SAH_GRAPHICS_BASE() :
    GRAPHICS_PREFS(),
        //GDATA(), 
	roll_angle(0),roll_phase(0),
	pitch_angle(0),pitch_phase(0),starfield(),seti_logo_texture(),
	user_logo_texture(), background_texture(),viewpoint_distance(1.0)
{
    data_struct_init();
}

void SAH_GRAPHICS_BASE::start_rotate() {
    if (roll_period == 0 && pitch_period == 0) return;
    glPushMatrix();
    if (pitch_period) {
        glRotated(pitch_angle + pitch_range*sin(pitch_phase), 1., 0., 0);
    }
    if (roll_period) {
        glRotated(roll_angle + roll_range*sin(roll_phase), 0., 1., 0);
    }
}

void SAH_GRAPHICS_BASE::incr_rotate(double dt) {
    if (mouse_down) return;
    pitch_phase += PI2*(dt/pitch_period);
    if (pitch_phase > PI2) pitch_phase -= PI2;
    roll_phase += PI2*(dt/roll_period);
    if (roll_phase > PI2) roll_phase -= PI2;
}

void SAH_GRAPHICS_BASE::end_rotate() {
    if (roll_period == 0 && pitch_period == 0) return;
    glPopMatrix();
}

// A note on our coordinates:
// X should be between -4 and 4
// Y should be between -3 and 3
// Z should be centered around zero (graph is -2 to +2)

void SAH_GRAPHICS_BASE::draw_pillars() {
	float pos[3];
	GLfloat violet[] = {0.6, 0.3, .8, 1.0};
	GLfloat white[] = {1.0, 1.0, 1.0, 1.0};	
	COLOR color;
    double lum = .5;
    double sat = .5;
    double hue = start_hue + hue_change/2.;
    if (hue > 1) hue -= 1;
    if (hue < 1) hue += 1;
    HLStoRGB(hue, lum, sat, color);	
    color.a = 1.0;
	GLfloat graphColor[4] = {color.r,color.g,color.b,color.a};

	pos[0] = pos[1] = pos[2] = 0;
	mode_shaded(graphColor);
	pos[0] = -3.50; pos[1] = 0;
	drawCylinder(false, pos, 7.00, .10);
	drawSphere(pos, .10);
	pos[0] = 3.5; drawSphere(pos, .1);

	pos[0] = -3.50; pos[1] = 2.50;
	drawCylinder(false, pos, 7.00, .10);
	drawSphere(pos, .10);
	pos[0] = 3.5; drawSphere(pos, .1);

	pos[0] = 1.00; pos[1] = 1.25;
	drawCylinder(false, pos, 2.50, .10);
	pos[0] = 3.5; drawSphere(pos, .1);

	pos[0] = 1.00; pos[1] = 0;
	drawCylinder(true, pos, 2.50, .10);
}

void set_viewpoint(double dist) {
    double x, y, z;
    x = 0;
    y = 3.0*dist;
    z = 11.0*dist;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        x, y, z,  // eye position
        0,-.8,0,      // where we're looking
        0.0, 1.0, 0.      // up is in positive Y direction
    );

}


static void initlights() {
   GLfloat ambient[] = {1., 1., 1., 1.0};
   GLfloat position[] = {-13.0, 6.0, 20.0, 1.0};
   GLfloat dir[] = {-1, -.5, -3, 1.0};
   glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
   glLightfv(GL_LIGHT0, GL_POSITION, position);
   glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, dir);
}

void SAH_GRAPHICS_BASE::render_logos() {    
	float pos[3] = {.03, .03, 0};
	float size[3] = {.5, .14, 0};
    bool any = seti_logo_texture.present || user_logo_texture.present;

    if (any) {
	    mode_ortho();
	    mode_unshaded();
    }

	//draw texture justified at bottom left relative to coords
    if (seti_logo_texture.present) {
        seti_logo_texture.draw(pos, size, ALIGN_BOTTOM, ALIGN_BOTTOM);
    }

	float pos2[3] = {.7, .03, 0};
	float size2[3] = {.27, .3, 0};
	//draw texture justified at bottom right relative to coords
    if (user_logo_texture.present) {
        user_logo_texture.draw(pos2, size2, ALIGN_TOP, ALIGN_BOTTOM);
    }
    if (any) {
	    ortho_done();
    }
}

void SAH_GRAPHICS_BASE::render_background() {
    float pos[3], size[3];
    // draw image behind graph, if any
    //
    if (background_texture.present) {
        mode_texture();
	    pos[0] = -3; pos[1] = -3; pos[2] = -2;
        size[0] = 6; size[1] = 6; size[2] = 0;
	    background_texture.draw(pos, size, ALIGN_CENTER, ALIGN_CENTER);
    }
}

void SAH_GRAPHICS_BASE::render_3d_graph(double time_of_day) {
    double elapsed_time,frac_done;

    if (rarray.start_time == 0) {
        rarray.start_time = time_of_day;
    }
    elapsed_time = time_of_day - rarray.start_time;
    if (elapsed_time >= grow_time) {
        frac_done = 1.0;
    } else {
        frac_done = elapsed_time/grow_time;
    }
    rarray.draw_axes();
	rarray.draw_labels();

    rarray.draw_part(frac_done);
    if (elapsed_time >= grow_time + hold_time) {
        memcpy(&rarray, &sah_shmem->rarray_data, sizeof(REDUCED_ARRAY_DATA));
        rarray.start_time = time_of_day;
        rarray.new_array();
    }
}


// this sets up the shared data structures,
// don't make any OpenGL calls from here
// This is called before BOINC initialization,
// so don't do any BOINC-related stuff
//
void SAH_GRAPHICS_BASE::data_struct_init() {
    defaults();
    srand(time(NULL));
}

// this is called from the graphics thread;
// it does OpenGL initialization.
// This is always called after init_worker_thread(),
// so the graphics prefs are in place.
//
void SAH_GRAPHICS_BASE::graphics_thread_init() {
    boinc_get_init_data(app_init_data);
    parse_project_prefs(app_init_data.project_preferences);

    glShadeModel(GL_SMOOTH);				// Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);		// Black Background
    glClearDepth(1.0f);					// Depth Buffer Setup
    glEnable(GL_DEPTH_TEST);				// Enables Depth Testing
    glDepthFunc(GL_LEQUAL);				// The Type Of Depth Testing To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
    initlights();

    int viewport[4];
    get_viewport(viewport);
	int w = viewport[2];
	int h = viewport[3];
	resize(w,h);
}

void SAH_GRAPHICS_BASE::resize(int w, int h) {    
	//float fontsize = (w/35.0f>14) ? 14 : w/35.0f;	
    //int weight = (700>w*35.0f/24.0f) ? 700 : w*35.0f/24.0f;    
	//listBase=MyCreateFont("Ariel",fontsize,weight);
	height=h;
	width=w;
	glViewport(0, 0, w, h);
}

void SAH_GRAPHICS_BASE::setup_given_prefs() {
	char filename[256];

    boinc_max_fps = max_fps;
    boinc_max_gfx_cpu_frac = max_cpu/100.;
    if (starfield_size) {
        starfield.build_stars(starfield_size, starfield_speed);
    }
    seti_logo_texture.present = false;
    user_logo_texture.present = false;
    background_texture.present = false;

    double fsize=0;
    boinc_resolve_filename(SETI_LOGO_FILENAME, filename, sizeof(filename));
    if (!file_size(filename,fsize) && (fsize>4096)) {
        seti_logo_texture.load_image_file(filename);
    } else {
        fprintf(stderr,"Warning: unable to load JPEG file. File size=%d\n",(int)fsize);
	    seti_logo_texture.present=false;
    }
    boinc_resolve_filename(USER_LOGO_FILENAME, filename, sizeof(filename));
    user_logo_texture.load_image_file(filename);
    boinc_resolve_filename(BACKGROUND_FILENAME, filename, sizeof(filename));
    background_texture.load_image_file(filename);
}
