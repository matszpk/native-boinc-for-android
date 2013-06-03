#include "sah_config.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include "parse.h"
#include "s_util.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"

analysis_config cfg;

int main(int argc, char **argv) {
   int  i=0;
   bool beta=false, sah=false;
   
   if (argc != 3) {
     fprintf(stderr,"Usage:\n  insert_analysis_config beta|sah filename\n");
     exit(1);
   }
   if (strcmp(argv[1], "beta") == 0) {
        beta = true;
        fprintf(stderr, "Inserting into the beta DB\n");
   } else if (strcmp(argv[1], "sah") == 0) {
        sah = true;
        fprintf(stderr, "Inserting into the sah DB\n");
   } else {
     fprintf(stderr,"Usage:\n  insert_analysis_config beta|sah filename\n");
     exit(1);
   }

   std::ifstream f(argv[2]);
   
   if (!f.is_open()) {
     fprintf(stderr,"File not found!\n");
     exit(1);
   }

   if(beta) {
        db_change("sah2b@sah_master_tcp");
    } else if(sah) {
        db_change("sah2@sah_master_tcp");
    } else {
        fprintf(stderr,"bad DB specification\n");
    }
   
   while (!f.eof()) {
     f >> cfg;
     if (!f.eof()) {
       i++;
       cfg.insert();
       fprintf(stderr, "SQLCODE is %d  last SQLCODE is %d\n", sql_error_code(), sql_last_error_code());
     }
   } 
  
   std::cout << "Found " << i << " analysis configs." << std::endl;
}       
     
