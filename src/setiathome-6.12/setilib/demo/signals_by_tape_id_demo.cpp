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

#include "setilib.h"

const char * from_db = "sah2@sci_master_tcp";

template<typename T>
int get_and_print (T& table, int tape_id);

int main(int argc, char ** argv) {

    gaussian gauss;
    spike sp;
    pulse pul;
    triplet trip;
    
    char type[32];
    int tape_id, retval;
    bool keepgoing = true;

    if (argc != 3) {
        fprintf(stderr, "Usage: signals_by_tape_id table_name tape_id (demo version!)\n");
        fprintf(stderr, "Output format:\nid: peak_power mean_power time ra decl q_pix freq detection_freq barycentric_freq fft_len chirp_rate rfi_checked rfi_found reserved\n"); 
        exit (1);
    } else {
        strcpy(type, argv[1]);
        tape_id = atoi(argv[2]);
    }

    retval = db_change(from_db);
    fprintf(stderr, "db_change(%s) returns %d\n", from_db, retval);

    while (keepgoing) {
      if (strcmp(type,"triplet") == 0) { retval = get_and_print (trip, tape_id); }
      else if (strcmp(type,"gaussian") == 0) { retval = get_and_print (gauss, tape_id); }
      else if (strcmp(type,"pulse") == 0) { retval = get_and_print (pul, tape_id); }
      else if (strcmp(type,"spike") == 0) { retval = get_and_print (sp, tape_id); }
      else {
        fprintf(stderr, "Don't recognize table: %s\n", type);
        exit(1);
        }
      if (!retval) keepgoing = false; 
      }

    exit (0);
}

template<typename T>
int get_and_print (T& table, int tape_id) {

  int retval;

  retval = get_next_signal_by_tape_id (table, tape_id);
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
