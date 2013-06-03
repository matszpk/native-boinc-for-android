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

#include "sah_config.h"
#include <string.h>
#include <unistd.h>
#include <string>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "util.h"
#include "parse.h"
#include "error_numbers.h"

#include "app_config.h"

const char* APP_CONFIG_FILE = "sah_config.xml";
//const char* APP_CONFIG_FILE = "sah_enhanced_config.xml";

int APP_CONFIG::parse(char* buf) {

    memset(this, 0, sizeof(APP_CONFIG));
    min_disk_free_pct=10;
    parse_str(buf, "<scidb_name>", scidb_name, sizeof(scidb_name));
    parse_int(buf, "<max_wus_ondisk>", max_wus_ondisk);
    parse_double(buf, "<min_disk_free_pct>", min_disk_free_pct);
    parse_int(buf, "<min_quorum>", min_quorum );
    parse_int(buf, "<target_nresults>", target_nresults);
    parse_int(buf, "<max_error_results>", max_error_results);
    parse_int(buf, "<max_success_results>", max_success_results);
    parse_int(buf, "<max_total_results>", max_total_results);
    parse_bool(buf, "<assim_check_rfi>", assim_check_rfi);
    if (match_tag(buf, "</config>")) return 0;
    return ERR_XML_PARSE;
}

int APP_CONFIG::parse_file(char* dir) {
    char* p;
    char path[256];
    int retval;

    sprintf(path, "%s/%s", dir, APP_CONFIG_FILE);
    retval = read_file_malloc(path, p);
    if (retval) return retval;
    return parse(p);
}


// main routine for testing
//int main(int argc, char** argv) {
//
//    APP_CONFIG sah_config;
//    int retval;
//
//    retval = sah_config.parse_file("..");
//    if (retval) {
//        fprintf(stderr, "Can't parse config file\n");
//    } else {
//        fprintf(stderr, "db name is %s\n", sah_config.scidb_name);
//    }
//}
