// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// Example graphics application, paired with uc2.C
// This demonstrates:
// - using shared memory to communicate with the worker app
// - reading XML preferences by which users can customize graphics
//   (in this case, select colors)
// - handle mouse input (in this case, to zoom and rotate)
// - draw text and 3D objects using OpenGL

#ifdef _WIN32
#include "boinc_win.h"
#else
#include <vector>
#include <cmath>
#endif

#include "boinc_api.h"
#include "graphics2.h"
#include "diagnostics.h"
#include "sah_gfx.h"

SAH_GRAPHICS sah_graphics;
APP_INIT_DATA sah_aid;

int main(int argc, char** argv) {
    boinc_init_graphics_diagnostics(BOINC_DIAG_DEFAULTS);
    boinc_parse_init_data_file();
    boinc_get_init_data(sah_aid);
    if (sah_aid.project_preferences) {
        sah_graphics.parse_project_prefs(sah_aid.project_preferences);
    }
    boinc_graphics_loop(argc, argv);
}
