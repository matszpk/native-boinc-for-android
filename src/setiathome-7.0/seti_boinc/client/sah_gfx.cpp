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

// Copyright 2003 Regents of the University of California


// Here's a summary of the various functions for initializing graphics
// and responding to user preferences changes:
//
// SAH_GRAPHICS::worker_thread_init()
//     (called from worker thread once, at start)
//
//     SAH_GRAPHICS_BASE::worker_thread_init
//      set prefs to defaults
//         parse_project_prefs() (from data already in mem)
//         initialize graph buffers
//
// app_graphics_thread_init
//     (called from SetMode when create window)
//
//     SAH_GRAPHICS::graphics_thread_init()
//         SAH_GRAPHICS_BASE::graphics_thread_init()
//             init_camera()
//             init_lights()
//             app_resize();
//         setup_given_prefs()
//
// app_graphics_reread_prefs
//     (called from event loop when get REREAD_PREFS msg
//      and we have an open window)
//
//     SAH_GRAPHICS::reread_prefs()
//         SAH_GRAPHICS_BASE::reread_prefs()
//             boinc_parse_init_data_file()
//             boinc_get_init_data()
//             parse_project_prefs()
//         setup_given_prefs()
//
// SAH_GRAPHICS::setup_given_prefs()
//     SAH_GRAPHICS_BASE::setup_given_prefs()
//         init starfield
//         init logos
//     initialize graph, progress bar structures

#include "sah_config.h"

#ifdef _WIN32
#include "boinc_win.h"
#include <gl/gl.h>           // Header File For The OpenGL32 Library
#include <gl/glu.h>          // Header File For The GLu32 Library
#if !defined(__MINGW32__) && !defined(_MSC_VER)
      #include <gl/glaux.h>        // Header File For The Glaux Library
#endif
#include <glut.h>
#endif

#ifndef _WIN32
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __APPLE_CC__
#include <Carbon/Carbon.h>
#include <DrawSprocket/DrawSprocket.h>
#include <AGL/agl.h>
#include <AGL/aglRenderers.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#endif

#ifdef HAVE_GL_H
#include "gl.h"
#elif defined(HAVE_GL_GL_H)
#include <GL/gl.h>
#elif defined(HAVE_OPENGL_GL_H)
#include <OpenGL/gl.h>
#endif

#ifdef HAVE_GLU_H
#include "glu.h"
#elif defined(HAVE_GL_GLU_H)
#include <GL/glu.h>
#elif defined(HAVE_OPENGL_GLU_H)
#include <OpenGL/glu.h>
#endif

#ifdef HAVE_GLUT_H
#include "glut.h"
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_OPENGL_GLUT_H)
#include <OpenGL/glut.h>
#elif defined(HAVE_GLUT_GLUT_H)
#include <GLUT/glut.h>
#endif
#endif

#include "util.h"
#include "str_util.h"
#include "str_replace.h"
#include "boinc_api.h"
#include "graphics2.h"
#include "gutil.h"
#include "timecvt.h"
#include "sah_gfx.h"

bool mouse_down = false;
int mouse_x, mouse_y;

static void ra_string(float x, char* p) {
    int hr = (int) x;
    float fmin = x-hr;
    int min = (int)(fmin*60);
    float fsec = fmin*60 - min;
    int sec = (int)(fsec*60);
    sprintf(p, "%2d hr %2d' %2d\" RA", hr, min, sec);
}

static void dec_string(float x, char* p) {
    int deg = (int) x;
    float fmin = x-deg;
    int min = (int)(fmin*60);
    float fsec = fmin*60 - min;
    int sec = (int)(fsec*60);
    sprintf(p, "%s%d deg %2d' %2d\" Dec", (x<0?"-":"+"), deg, abs(min), abs(sec));
}

void SAH_GRAPHICS::init_text_panels() {
    float pos[3], size[2];
    COLOR color;
    double dtheta;
    double char_height = 0.15;
    double line_width = 2.0;
    double line_spacing = 0.3;
    double margin = 0.2;
    color.a = graph_alpha;
    double lum = .6;
    double sat = .9;

    double hue = start_hue + hue_change*.33;
    HLStoRGB(hue, lum, sat, color);

    pos[0] = -1;
    pos[1] = -1;
    pos[2] = -2;
    size[0] = 4.9;
    size[1] = 3.;
    dtheta = .3;
    text_panels[0].init(
        pos, size, color, dtheta, char_height, line_width, line_spacing, margin
    );

    hue = start_hue + hue_change*.66;
    if (hue > 1) hue -= 1;
    if (hue < 1) hue += 1;
    HLStoRGB(hue, lum, sat, color);
    pos[0] = -3;
    pos[1] = 0;
    pos[2] = -1.8;
    dtheta = -.6;
    text_panels[1].init(
        pos, size, color, dtheta, char_height, line_width, line_spacing, margin
    );
}

void SAH_GRAPHICS::get_data_info_string(char* buf) {
    char ra_buf[256], dec_buf[256];
    static const char *locations[]= {
        "Arecibo 403MHz Line Feed",
        "Arecibo 1.42GHz Flat Feed",
        "Arecibo 1.42GHz Gregorian Feed",
        "Unknown"
    };
    int s4_id=3;

    if (gdata->ready) {
        s4_id = gdata->wu.s4_id;
        // starting after the 3rd receiver_cfg entry, the long form
        // name appears in receiver_cfg->name
        if (s4_id>2) {
          if (strlen(gdata->wu.receiver_name)==0) {
            locations[0]="SETI@home Multi-Beam";
          } else {
            locations[0]=gdata->wu.receiver_name;
          }
          s4_id=0;
        }
        ra_string(gdata->wu.start_ra, ra_buf);
        dec_string(gdata->wu.start_dec, dec_buf);
        sprintf(buf, "From: %s, %s\n"
            "Recorded on: %s\n"
            "Recorded at: %s\n"
            "Base frequency: %.9f GHz",
            ra_buf, dec_buf,
            short_jd_string(gdata->wu.time_recorded),
            locations[s4_id],
            gdata->wu.subband_base/1e9
        );
    } else {
        strcpy( buf, "" );
    }
}

void SAH_GRAPHICS::get_user_info_string(char* buf) {
    sprintf( buf,
        "Name: %s\n"
        "Team: %s\n"
        "Total credit: %.2f\n",
        app_init_data.user_name,
        app_init_data.team_name,
        app_init_data.user_total_credit
    );
}

void SAH_GRAPHICS::get_analysis_info_string(char* buf) {
    if (gdata->ready) {
        sprintf(buf,
            "%s\n"
            "Doppler drift rate %6.4f Hz/sec  Resolution %5.3f Hz\n",
            gdata->status,
            gdata->fft_info.chirp_rate, gdata->wu.subband_sample_rate/gdata->fft_info.fft_len
        );
    } else {
        strcpy( buf, "" );
    }
}

// decide what type of signal to display.
// If there's anything new (dirty) display it.
// Otherwise cycle through the types
//
int SAH_GRAPHICS::choose_signal_to_display(double time_of_day) {
    int choices[3], nchoices;

    if (gdata->gi.dirty) {
        gdata->gi.dirty = false;
        return 0;
    } else if (gdata->pi.dirty) {
        gdata->pi.dirty = false;
        return 1;
    } else if (gdata->ti.dirty) {
        gdata->ti.dirty = false;
        return 2;
    } else {
        nchoices = 0;
        if (gdata->gi.peak_power) {
            choices[nchoices++] = 0;
        }
        if (gdata->pi.peak_power) {
            choices[nchoices++] = 1;
        }
        if (gdata->ti.peak_power) {
            choices[nchoices++] = 2;
        }
        if (nchoices == 0) return -1;
        if (nchoices == 1) return choices[0];
        int i = (int)fmod(time_of_day/5., (double)nchoices);      // rotate every 5 seconds
        return choices[i];
    }
}

void SAH_GRAPHICS::render_pillars(double time_of_day, double dt) {
    int i;
    float pos[3];
    double cur_cpu_time;
    double max = 0, d;
    float pulseData[PULSE_POT_LEN];
    float tripletData[TRIPLET_POT_LEN];
    float gaussFunc[GAUSS_POT_LEN];
    char buf[512],time_buf[256];
    const char *TitleText="SETI@home 7";


    int s4_id=3;

    if (gdata && gdata->ready) {
        s4_id=gdata->wu.s4_id;
        if (s4_id > 2) {
            TitleText="SETI@home Version 7";
        }
    }

    draw_pillars();

    // draw User and Data text
    //
    mode_lines();

    glColor3f(1., 1., 1.);                  // white text
    pos[0] = 2;
    pos[1] = 2.15;
    pos[2] = 0;
    draw_text_line(pos, 0.2, 2.0, "Data info");
    pos[0] = 1.2;
    pos[1] -= 0.2;

    get_data_info_string(buf);

    draw_text(pos, 0.1, 1.0, 0.15, buf );
    pos[0] = 2;
    pos[1] = .8;
    draw_text_line(pos, 0.2, 2.0, "User info");
    pos[0] = 1.2;
    pos[1] -= .2;

    get_user_info_string(buf);
    draw_text(pos, 0.1, 1.0, 0.15, buf);
    pos[0] = -2.75;
    pos[1] = 2.15;
    draw_text_line(pos, 0.2, 2.0, TitleText);
    pos[0] = -3.5;
    pos[1] = 1.9;

    get_analysis_info_string(buf);
    draw_text(pos, 0.1, 1.0, 0.15, buf);

    // draw CPU time and progress bars
    //
    pos[1] = 0.3;
    cur_cpu_time=gdata->cpu_time;
    ndays_to_string(cur_cpu_time/SECONDS_PER_DAY, 0, time_buf );
    sprintf(buf, "Overall %.3f%% done    CPU time: %s", floor(gdata->progress*100000)/1000, time_buf);
    draw_text_line(pos, 0.1, 1.0, buf);
    mode_unshaded();

    inner_progress.draw(gdata->local_progress);
    outer_progress.draw(gdata->progress);

    // draw current or best signal
    //

    if (!gdata->ready) return;
    glColor3f(1., 1., 1.);
    mode_unshaded();

    pos[0] = -3.5;
    pos[1] = 1.6;
    pos[2] = 0.;

    switch (choose_signal_to_display(time_of_day)) {
        case 0:
            if (gdata->gi.peak_power) {
                sprintf( buf, "%sGaussian: power %5.2f, fit %5.3f, score %5.3f",
                         gdata->gi.is_best?"Best ":"New ",
                         gdata->gi.peak_power, gdata->gi.chisqr, gdata->gi.score );
                draw_text(pos, 0.1, 1.0, 0.15, buf);

                for (i=0;i<GAUSS_POT_LEN;i++) {
                    if (gdata->gi.pot[i] > max) max = gdata->gi.pot[i];
                }
                for (i=0;i<GAUSS_POT_LEN;i++) {
                    d = (i - gdata->gi.fft_ind)/gdata->gi.sigma;
                    gaussFunc[i] = (gdata->gi.mean_power + gdata->gi.peak_power*exp(-d*d*0.693))/max;
                }
                rnd_graph.draw(gdata->gi.pot, GAUSS_POT_LEN);
                sin_graph.draw(gaussFunc, GAUSS_POT_LEN);
            }
            break;
        case 1:
            if (gdata->pi.peak_power) {
                sprintf( buf, "%sPulse: power %5.2f, period %6.4f, score %5.2f",
                         gdata->pi.is_best?"Best ":"New ",
                         gdata->pi.peak_power, gdata->pi.period, gdata->pi.score );
                draw_text(pos, 0.1, 1.0, 0.15, buf);

                for (i=0;i<PULSE_POT_LEN;i++) {
                    pulseData[i] = gdata->pi.pot_max[i]/255.0;
                }
                rnd_graph.draw(pulseData, PULSE_POT_LEN);
            }
            break;
        case 2:
            if (gdata->ti.peak_power) {
                sprintf(
                    buf, "%sTriplet: power %5.2f, period %6.4f",
                    gdata->ti.is_best?"Best ":"New ",
                    gdata->ti.peak_power, gdata->ti.period
                );
                draw_text(pos, 0.1, 1.0, 0.15, buf);
                for (i=0;i<TRIPLET_POT_LEN;i++) {
                    tripletData[i] = gdata->ti.pot_max[i]/255.0;
                }
                rnd_graph.add_tick(gdata->ti.tpotind0_0, 0);
                rnd_graph.add_tick(gdata->ti.tpotind1_0, 1);
                rnd_graph.add_tick(gdata->ti.tpotind2_0, 2);
                rnd_graph.draw(tripletData, TRIPLET_POT_LEN, true);
            }
            break;
    }
}
extern float* white;
void SAH_GRAPHICS::render_headsup(double time_of_day, double dt) {
    double cur_cpu_time;
    COLOR color;
    color.a = .7;
    double lum = .5;
    double sat = .5;
    double hue = start_hue + hue_change/2.;
    if (hue > 1) hue -= 1;
    if (hue < 1) hue += 1;
    HLStoRGB(hue, lum, sat, color);

    mode_ortho();
    GLfloat char_height = 0.015;
    GLfloat line_width = 0.25;
    GLfloat line_spacing = 0.03;
    GLfloat margin = 0.2;
    char buf[512], time_buf[256];

    float pos[3] = {0, 0.7, 1.0};

    mode_unshaded();

    //float pos1[3] = {.03,.94,1.0};
    //float pos2[3] = {.97,.94,1.0};
    float pos1[3] = {0, .62, 1.0};
    float pos2[3] = {1, .71, 1.0};

    get_analysis_info_string(buf);
    draw_text(pos1, char_height, line_width, line_spacing, buf);

    get_user_info_string(buf);
    get_data_info_string(time_buf);
    strcat(buf, time_buf);
    draw_text_right(pos2, char_height, line_width, line_spacing, buf);

    cur_cpu_time=gdata->cpu_time;
    ndays_to_string(cur_cpu_time/SECONDS_PER_DAY, 0, time_buf );
    sprintf(buf, "Overall %.3f%% done    CPU time: %s", floor(gdata->progress*100000)/1000, time_buf);
    draw_text(pos, char_height, line_width, line_spacing, buf);

    outer_progress_2d.draw(gdata->progress);
    glColor4f(1,1,1,1);

    if (gdata->ready) {
        double max = 0, d;
        float pulseData[PULSE_POT_LEN];
        float tripletData[TRIPLET_POT_LEN];
        float gaussFunc[GAUSS_POT_LEN];
        char buf[256];
        int choice;
        int i;

        choice = choose_signal_to_display(time_of_day);
        pos[1]=.47;
        switch (choice) {
            case 0:
                if (gdata->gi.peak_power) {
                    sprintf( buf, "%sGaussian: power %5.2f, fit %5.3f, score %5.3f",
                             gdata->gi.is_best?"Best ":"",
                             gdata->gi.peak_power, gdata->gi.chisqr, gdata->gi.score );
                    draw_text(pos, char_height, line_width, line_spacing, buf);

                    for (i=0;i<GAUSS_POT_LEN;i++) {
                        if (gdata->gi.pot[i] > max) max = gdata->gi.pot[i];
                    }
                    for (i=0;i<GAUSS_POT_LEN;i++) {
                        d = (i - gdata->gi.fft_ind)/gdata->gi.sigma;
                        gaussFunc[i] = (gdata->gi.mean_power + gdata->gi.peak_power*exp(-d*d*0.693))/max;
                    }
                    sin_graph.draw(gaussFunc, GAUSS_POT_LEN);
                    rnd_graph.draw(gdata->gi.pot, GAUSS_POT_LEN);
                }
                break;
            case 1:
                if (gdata->pi.peak_power) {
                    sprintf(
                        buf, "%sPulse: power %5.2f, period %6.4f, score %5.2f",
                        gdata->pi.is_best?"Best ":"",
                        gdata->pi.peak_power, gdata->pi.period, gdata->pi.score
                    );
                    draw_text(pos, char_height, line_width, line_spacing, buf);

                    for (i=0;i<PULSE_POT_LEN;i++) {
                        pulseData[i] = gdata->pi.pot_max[i]/255.0;
                    }
                    rnd_graph.draw(pulseData, PULSE_POT_LEN);
                }
                break;
            case 2:
                if (gdata->ti.peak_power) {
                    sprintf(
                        buf, "Best Triplet: power %5.2f, period %6.4f",
                        gdata->ti.peak_power, gdata->ti.period
                    );
                    draw_text(pos, char_height, line_width, line_spacing, buf);
                    for (i=0;i<TRIPLET_POT_LEN;i++) {
                        tripletData[i] = gdata->ti.pot_max[i]/255.0;
                    }
                    rnd_graph.add_tick(gdata->ti.tpotind0_0, 1 );
                    rnd_graph.add_tick(gdata->ti.tpotind1_0, 1 );
                    rnd_graph.add_tick(gdata->ti.tpotind2_0, 1 );
                    rnd_graph.draw(tripletData, TRIPLET_POT_LEN);
                }
                break;
        }
    }

    ortho_done();
}

void SAH_GRAPHICS::render_panels(double time_of_day, double dt) {
    double cur_cpu_time;
    char buf[512],time_buf[256];

    float pos[3] = {0., 0., 0.};
    int i;
    MOVING_TEXT_PANEL tp[2];

    get_user_info_string(buf);
    get_data_info_string(time_buf);
    strcat(buf, time_buf);
    text_panels[0].set_text(0, buf);

    get_analysis_info_string(buf);
    text_panels[1].set_text(0, buf);

    cur_cpu_time=gdata->cpu_time;
    ndays_to_string(cur_cpu_time/SECONDS_PER_DAY, 0, time_buf );
    sprintf(buf, "Overall %.3f%% done    CPU time: %s", floor(gdata->progress*100000)/1000, time_buf);
    text_panels[1].set_text(6, buf);

    text_panels[1].get_pos(8, pos);
    pos[2]+=.01;
    outer_progress_2d.set_pos(pos);

    pos[0] = 0.03;
    pos[1] = 0.76;
    pos[2] = 0;

    mode_unshaded();

    outer_progress_2d.draw(gdata->progress);
    glColor4f(1,1,1,1);

    if (gdata->ready) {
        double max = 0, d;
        float pulseData[PULSE_POT_LEN];
        float tripletData[TRIPLET_POT_LEN];
        //float pos[3] = {.03, .94-(2.*line_spacing), 0.};
        float gaussFunc[GAUSS_POT_LEN];
        char buf[256];
        int choice;
        int i;

        choice = choose_signal_to_display(time_of_day);
        strcpy(buf, "");
        switch (choice) {
            case 0:
                if (gdata->gi.peak_power) {
                    sprintf( buf, "%sGaussian: power %5.2f, fit %5.3f, score %5.3f",
                             gdata->gi.is_best?"Best ":"",
                             gdata->gi.peak_power, gdata->gi.chisqr, gdata->gi.score
                           );

                    for (i=0;i<GAUSS_POT_LEN;i++) {
                        if (gdata->gi.pot[i] > max) max = gdata->gi.pot[i];
                    }
                    for (i=0;i<GAUSS_POT_LEN;i++) {
                        d = (i - gdata->gi.fft_ind)/gdata->gi.sigma;
                        gaussFunc[i] = (gdata->gi.mean_power + gdata->gi.peak_power*exp(-d*d*0.693))/max;
                    }

                    text_panels[1].get_pos(4, pos);
                    rnd_graph.set_pos(pos);
                    text_panels[1].get_pos(4, pos);
                    pos[2] -= -.01;
                    sin_graph.set_pos(pos);
                    sin_graph.draw(gaussFunc, GAUSS_POT_LEN);
                    rnd_graph.draw(gdata->gi.pot, GAUSS_POT_LEN);
                }
                break;
            case 1:
                if (gdata->pi.peak_power) {
                    sprintf( buf, "%sPulse: peak_power %5.2f, period %6.4f, score %5.2f",
                             gdata->pi.is_best?"Best ":"",
                             gdata->pi.peak_power, gdata->pi.period, gdata->pi.score
                           );

                    for (i=0;i<PULSE_POT_LEN;i++) {
                        pulseData[i] = gdata->pi.pot_max[i]/255.0;
                    }

                    text_panels[1].get_pos(4, pos);
                    rnd_graph.set_pos(pos);
                    rnd_graph.draw(pulseData, PULSE_POT_LEN);
                }
                break;
            case 2:
                if (gdata->ti.peak_power) {
                    sprintf(
                        buf, "Best Triplet: power %5.2f, period %6.4f",
                        gdata->ti.peak_power, gdata->ti.period
                    );
                    for (i=0;i<TRIPLET_POT_LEN;i++) {
                        tripletData[i] = gdata->ti.pot_max[i]/255.0;
                    }

                    text_panels[1].get_pos(4, pos);
                    rnd_graph.set_pos(pos);

                    rnd_graph.add_tick(gdata->ti.tpotind0_0, 0);
                    rnd_graph.add_tick(gdata->ti.tpotind1_0, 1);
                    rnd_graph.add_tick(gdata->ti.tpotind2_0, 2);
                    rnd_graph.draw(tripletData, TRIPLET_POT_LEN);
                }
                break;
        }
        text_panels[1].set_text(5, buf);
    }
    for (i=0; i<2; i++) {
        tp[i] = text_panels[i];
    }

    MOVING_TEXT_PANEL::sort(tp, 2);
    for (i=0; i<2; i++) {
        tp[i].draw();
    }
    for (i=0; i<2; i++) {
        text_panels[i].move(dt);
    }

}

void app_graphics_render(int x, int y, double t) {
    sah_graphics.render(x, y, t);
}

void app_graphics_init() {
    sah_graphics.graphics_thread_init();
}

void SAH_GRAPHICS::data_struct_init() {
    SAH_GRAPHICS_BASE::data_struct_init();
}

// Name is misleading - this gets called each time the app switches mode
//
void SAH_GRAPHICS::graphics_thread_init() {
    SAH_GRAPHICS_BASE::graphics_thread_init();
    setup_given_prefs();
}

void SAH_GRAPHICS::setup_given_prefs() {
    float pos[3], size[3];
    float outer[] = {1.0, 0., 0.2, 0.4};
    float inner[] = {0.2, 0.0, 0.7, 0.9};

    float red[] = {1., 0., 0., 1.};
    float green[] = {0., 1., 0., 1.};
    float white[] = {1., 1., 1., 1.};
    float reda[] = {1., 0, 0, .8};

    pos[0] = -3.5;
    pos[1] = 0.8;
    pos[2] = -.4;
    size[0] = 4.;
    size[1] = .5;
    size[2] = .4;

    SAH_GRAPHICS_BASE::setup_given_prefs();

    switch (text_style) {
        case TEXT_STYLE_PILLARS:
            rnd_graph.init(pos, size, red, green);
            pos[2] = -.2;
            size[2] = .1;
            sin_graph.init(pos, size, white, white);
            pos[0] = -1.5;
            pos[1] = 1.95;
            pos[2] = 0;
            inner_progress.init(pos, 1.0, 0.07, 0.05, outer, inner);

            pos[0] = -3.5;
            pos[1] = 0.60;
            pos[2] = 0;
            outer_progress.init(pos, 4., 0.1, 0.08, outer,inner);
            break;
        case TEXT_STYLE_HEADSUP:
            pos[0] = .03;
            pos[1] = .5;
            pos[2] = 1;
            size[0] = .4;
            size[1] = .08;
            size[2] = 0;
            rnd_graph.init(pos, size, reda, green);
            size[2]=0;
            pos[2]=0;
            sin_graph.init(pos, size, white, white);
            pos[0] = .03;
            pos[1] = 0.67;
            pos[2] = 0;
            outer_progress_2d.init(pos,.4,.02,.015,outer,inner);
            break;
        case TEXT_STYLE_PANELS:
            init_text_panels();
            size[0] = 3;
            size[1] = .5;
            size[2] = .1;
            rnd_graph.init(pos, size, reda, green);
            size[2]=.05;
            sin_graph.init(pos, size, white, white);
            pos[0] = .03;
            pos[1] = .745;
            pos[2] = 0;
            outer[3]=.7;
            outer_progress_2d.init(pos,3,.2,.15,outer,inner);
            break;
    }

    pos[0] = -3.50;
    pos[1] = -2.50;
    pos[2] = -2.00;
    size[0] = 7.00;
    size[1] = 2.00;
    size[2] = 4.00;
    rarray.init_display(
        graph_style, pos, size, start_hue, hue_change, graph_alpha,
        "Frequency (Hz)","Power","Time (sec)"
    );
}

void app_graphics_resize(int w, int h) {
    sah_graphics.resize(w, h);
}

void boinc_app_mouse_move(int x, int y, int left, int middle, int right) {
    if (left) {
        sah_graphics.pitch_angle += (y-mouse_y)*.1;
        sah_graphics.roll_angle += (x-mouse_x)*.1;
        mouse_y = y;
        mouse_x = x;
    } else if (right) {
        double d = (y-mouse_y);
        sah_graphics.viewpoint_distance *= exp(d/100.);
        mouse_y = y;
        mouse_x = x;
    } else {
        mouse_down = false;
    }
}

void boinc_app_mouse_button(int x, int y, int which, int is_down) {
    if (is_down) {
        mouse_down = true;
        mouse_x = x;
        mouse_y = y;
    } else {
        mouse_down = false;
    }
}

void boinc_app_key_press(int, int) {}

void boinc_app_key_release(int, int) {}

static void app_init_camera(double dist) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        45.0,       // field of view in degree
        1.0,        // aspect ratio
        1.0,        // Z near clip
        1000.0     // Z far
    );

    set_viewpoint(dist);
}

void SAH_GRAPHICS::render(int xs, int ys, double time_of_day) {
    static double last_time=0;
    double dt = 0;

    if (!sah_shmem) {
        sah_shmem = (SAH_SHMEM*)boinc_graphics_get_shmem("setiathome");
        gdata = &(sah_shmem->gdata);
    }
    gdata->countdown = 5;

    if (last_time != 0) {
        dt = time_of_day - last_time;
    }
    last_time = time_of_day;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (starfield_size) {
        starfield.update_stars(dt);
    }

    render_logos();
    app_init_camera(viewpoint_distance);
    scale_screen(width, height);

    start_rotate();
    incr_rotate(dt);

    render_background();

    switch (text_style) {
        case TEXT_STYLE_PILLARS:
            render_pillars(time_of_day, dt);
            break;
        case TEXT_STYLE_PANELS:
            render_panels(time_of_day, dt);
            break;
        case TEXT_STYLE_HEADSUP:
            render_headsup(time_of_day, dt);
            break;
    }
    render_3d_graph(time_of_day);
    end_rotate();
    glFlush();
}
