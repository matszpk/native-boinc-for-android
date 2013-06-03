// NOTE!!! This is the demo version of this program, mostly here as a good
// example of basic seti/db library usage and to test your compiler.
// You should find the real version of this in seti_science. - mattl 9/19/07

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "db_table.h"
#include "schema_master.h"
#include "sqlhdr.h"

const char *from_db = "sah2@sci_master_tcp";

template<typename T>
int get_next_signal_by_id_range (T &table, int minid, int maxid);

template<typename T>
int get_and_print (T &table, int minid, int maxid);

int main(int argc, char **argv) {

    gaussian gauss;
    spike sp;
    pulse pul;
    triplet trip;

    char type[32];
    int minid, maxid, retval;
    bool keepgoing = true;

    if (argc != 4) {
        fprintf(stderr, "Usage: signals_by_id_range table_name minimum_id maximum_id (demo version!)\n");
        fprintf(stderr, "Output format:\nid: peak_power mean_power time ra decl q_pix freq detection_freq barycentric_freq fft_len chirp_rate rfi_checked rfi_found reserved\n");
        exit (1);
    } else {
        strcpy(type, argv[1]);
        minid = atoi(argv[2]);
        maxid = atoi(argv[3]);
    }

    retval = db_change(from_db);
    fprintf(stderr, "db_change(%s) returns %d\n", from_db, retval);

    while (keepgoing) {
        if (strcmp(type,"triplet") == 0) {
            retval = get_and_print (trip, minid, maxid);
        } else if (strcmp(type,"gaussian") == 0) {
            retval = get_and_print (gauss, minid, maxid);
        } else if (strcmp(type,"pulse") == 0) {
            retval = get_and_print (pul, minid, maxid);
        } else if (strcmp(type,"spike") == 0) {
            retval = get_and_print (sp, minid, maxid);
        } else {
            fprintf(stderr, "Don't recognize table: %s\n", type);
            exit(1);
        }
        if (!retval) {
            keepgoing = false;
        }
    }

    exit (0);
}

template<typename T>
int get_next_signal_by_id_range (T &table, int minid, int maxid) {

    static bool start = true;
    int retval, get_next_retval;
    int i, match, dontmatch;

    char c_string[256];
    std::string where_clause;

    sprintf(c_string, "where id >= %d and id <= %d",  minid, maxid);
    where_clause = c_string;

    if (start) {
        retval = table.open_query(where_clause);
        if (retval) {
            fprintf(stderr, "open_query returns %d, %d\n", retval, sql_last_error_code());
        }
        start = false;
    }
    get_next_retval = table.get_next();
    return get_next_retval;
}

template<typename T>
int get_and_print (T &table, int minid, int maxid) {

    int retval;

    retval = get_next_signal_by_id_range (table, minid, maxid);
    std::cout.precision(14);
    if (retval) std::cout << table.id << " : " << table.peak_power << " "
                << table.mean_power << " "
                << table.time << " "
                << table.ra << " "
                << table.decl << " "
                << table.q_pix << " "
                << table.freq << " "
                << table.detection_freq << " "
                << table.barycentric_freq << " "
                << table.fft_len << " "
                << table.chirp_rate << " "
                << table.rfi_checked << " "
                << table.rfi_found << " "
                << table.reserved << "\n";
    return retval;

}
