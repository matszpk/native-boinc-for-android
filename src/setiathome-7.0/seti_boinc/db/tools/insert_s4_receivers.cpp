#include "sah_config.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <vector>
#include "s_util.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"

#include <s4tel.h>
#include <s4cfg.h>

#define S4_RECEIVER_CONFIG_FILE "/usr/local/warez/projects/s4/siren/db/ReceiverConfig.tab"

ReceiverConfig_t input;
receiver_config  output;

int main() {
   int i,j;
   if (!sql_database("sah2b@sci_master_tcp")) exit(1);
   if (!getenv("S4_RECEIVER_CONFIG"))
	putenv("S4_RECEIVER_CONFIG="S4_RECEIVER_CONFIG_FILE);

   for (i=0;i<3;i++)  {
     memset(&input,0,sizeof(input));
     s4cfg_GetReceiverConfig(i, &input);
     output.s4_id=input.ReceiverID;
     strncpy(output.name,input.ReceiverName,255);
     output.name[254]=0;
     output.beam_width=input.BeamWidth;
     output.center_freq=input.CenterFreq;
     output.latitude=input.Latitude;
     output.longitude=input.Longitude;
     output.elevation=input.Elevation;
     output.diameter=input.Diameter;
     output.az_orientation=input.AzOrientation;
     output.zen_corr_coeff.clear();
     output.az_corr_coeff.clear();
     for (j=0;j<13;j++) {
       output.zen_corr_coeff.push_back(input.ZenCorrCoeff[j]);
       output.az_corr_coeff.push_back(input.AzCorrCoeff[j]);
     }
     db_change("sah2b@sci_master_tcp");
     output.id=0;
     if (output.insert()) {
       std::cout << "inserted row (id=" << output.id << ")" << std::endl;
     } else {
       std::cout << "insert failed, sql_error_code =" << sql_error_code() << " sql_last_error_code =" << sql_last_error_code() << std::endl;
     }
   }
}       
     
