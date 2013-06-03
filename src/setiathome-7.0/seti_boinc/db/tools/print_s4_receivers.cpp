#include "sah_config.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <vector>
#include "s_util.h"
#include "sqlapi.h"
#include "xml_util.h"
#include "db_table.h"
#include "schema_master.h"

receiver_config input;

int main() {
   if (!sql_database("sah2b@sci_master_tcp")) exit(1);
   std::cout << "<receiver_array>\n";
   std::cout << xml_indent() << "<row_count>" <<  input.count() << "</row_count>\n" ;
   if (input.open_query()) {
     while (input.get_next()) {
       std::cout << input.print_xml();
     }
   }
   std::cout << "</receiver_array>\n";
}
