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

class APP_CONFIG {
public:
    char scidb_name[256];
    int  max_wus_ondisk;        // actually the number of unsent results
    double min_disk_free_pct;   // Minimum free disk percent in download dir.

    // fields in the boinc DB workunit record
    int min_quorum;             // minimum quorum size
    int target_nresults;	// try to get this many successful results
    int max_error_results;      // WU error if < #error results
    int max_total_results;      // WU error if < #total results
    int max_success_results;    // WU error if < #success results
    bool assim_check_rfi;       // if true, assimilator will check each signal for rfi

    int parse(char*);
    int parse_file(char* dir=".");
};
