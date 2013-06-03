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

receiver_config cfg;

int main(int argc, char **argv) {
   int  i=0;
   
   if (argc != 2) {
     fprintf(stderr,"Usage:\n  insert_receiver_config filename\n");
     exit(1);
   }

   std::ifstream f(argv[1]);
   
   if (!f.is_open()) {
     fprintf(stderr,"File not found!\n");
     exit(1);
   }

   db_change("sah2@sci_master_tcp");
   
   while (!f.eof()) {
     f >> cfg;
     if (!f.eof()) {
       i++;
       cfg.insert();
     }
   } 
  
   std::cout << "Found " << i << " receiver configs." << std::endl;
}       
    
