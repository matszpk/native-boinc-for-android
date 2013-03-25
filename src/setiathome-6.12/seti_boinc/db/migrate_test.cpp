#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>

#include "db_table.h"
#include "schema_master.h"
#include "sqlhdr.h"

const char * from_db = "sah2@sci_master_shm";
const char * to_db = "t_sah@temp_master_tcp";

template<typename T>
int read_and_compare_dbs (T& table, int modulo, int remainder, int maxnum, int minid, int maxid);

int main(int argc, char ** argv) {

    receiver_config receiver_cfg;
    recorder_config recorder_cfg; 
    splitter_config splitter_cfg;
    analysis_config analysis_cfg;
    tape tp;
    settings set;
    workunit_grp wug;
    workunit_header wuh;
    result res;
    triplet trip;
    gaussian gauss;
    pulse pul;
    spike sp;
    
    char type[32];
    int modulo, remainder, maxnum, minid, maxid;
          

    if (argc != 7) {
        fprintf(stderr, "Usage: migrate_test table_name modulo remainder maximum_number_of_elements minimum_id maximum_id\n");
        exit (1);
    } else {
        strcpy(type, argv[1]);
        modulo = atoi(argv[2]);
        remainder = atoi(argv[3]);
        maxnum = atoi(argv[4]);
        minid = atoi(argv[5]);
        maxid = atoi(argv[6]);
    }

    // how to call read_and_compare_dbs?

    if (strcmp(type,"receiver_config") == 0) { read_and_compare_dbs (receiver_cfg, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"recorder_config") == 0) { read_and_compare_dbs (recorder_cfg, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"splitter_config") == 0) { read_and_compare_dbs (splitter_cfg, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"analysis_config") == 0) { read_and_compare_dbs (analysis_cfg, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"tape") == 0) { read_and_compare_dbs (tp, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"settings") == 0) { read_and_compare_dbs (set, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"workunit_grp") == 0) { read_and_compare_dbs (wug, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"workunit_header") == 0) { read_and_compare_dbs (wuh, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"result") == 0) { read_and_compare_dbs (res, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"triplet") == 0) { read_and_compare_dbs (trip, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"gaussian") == 0) { read_and_compare_dbs (gauss, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"pulse") == 0) { read_and_compare_dbs (pul, modulo, remainder, maxnum, minid, maxid); }
    else if (strcmp(type,"spike") == 0) { read_and_compare_dbs (sp, modulo, remainder, maxnum, minid, maxid); }
    else {
      fprintf(stderr, "Don't recognize table: %s\n", type);
      exit(1);
      }
    exit (0);
}


template<typename T>
int read_and_compare_dbs (T& table, int modulo, int remainder, int maxnum, int minid, int maxid) {
  
    int retval, get_next_retval;
    int i, match, dontmatch;
    std::vector<int> id_list;

    char c_string[256]; 
    std::string where_clause;
 
    if (remainder != 0) { 
      sprintf(c_string, "where id >= %d and id <= %d and MOD(id, %d) = %d",  minid, maxid,  modulo, remainder);
    } else {
      sprintf(c_string, "where id >= %d and id <= %d",  minid, maxid);
    }
    where_clause = c_string;
    std::cerr << "where clause: " << where_clause << "\n";

    retval = db_change(from_db);
    fprintf(stderr, "db_change(%s) returns %d\n", from_db, retval);
    retval = table.open_query(where_clause);
    fprintf(stderr, "open_query returns %d, %d\n", retval, sql_last_error_code());
    i = 0;
    while (get_next_retval = table.get_next() && (i++) < maxnum) id_list.push_back (table.id);
    table.close_query();

    typename std::vector<int>::iterator id_iter = id_list.begin();
    T from_thisrow;
    T to_thisrow;

    match = dontmatch = 0;
    while (id_iter != id_list.end()) {
      sprintf(c_string, "where id = %d", (*id_iter));
      where_clause = c_string;
      db_change(from_db);
      table.open_query(where_clause);
      table.get_next();
      from_thisrow = table;
      db_change(to_db);
      table.open_query(where_clause);
      table.get_next();
      to_thisrow = table;
      std::cerr << "FROM DATABASE:\n" << from_thisrow.print(0,1,0) << "\n\nTO DATABASE:\n" << to_thisrow.print(0,1,0) << "\n";
      if (from_thisrow.print(0,1,0) == to_thisrow.print(0,1,0)) { match++; }
      else { 
        dontmatch++;
        fprintf(stderr,"DON'T MATCH!!!\n");
      }  
      fprintf(stderr,"--------------------------\n");
      id_iter++;
    } 
    fprintf(stderr,"num match: %d don't match: %d\n",match,dontmatch);
    return 0;
}
