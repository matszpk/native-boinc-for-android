#include <map>
#include <math.h>
#include "setilib.h"
#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"

coordinate_t cmap_interp(std::map<seti_time,coordinate_t> &map, seti_time &t) {
  std::map<seti_time,coordinate_t>::iterator above(map.upper_bound(t));
  std::map<seti_time,coordinate_t>::iterator below;
  below=above;
  if (above==map.begin()) {
    above++;
  } else {
    below--;
  }
  if (above==map.end()) {
    above--;
    below--;
  }
  coordinate_t upper(above->second);
  coordinate_t lower(below->second);
  coordinate_t rv;
  
  if ((upper.ra-lower.ra)>23) lower.ra+=24;
  if ((lower.ra-upper.ra)>23) upper.ra+=24;
  rv.time=t.jd().uval();
  double f=(seti_time(t.jd()-JD1970,JD1970)-seti_time(days(lower.time)))/
                days(upper.time-lower.time);
  rv.ra=(upper.ra-lower.ra)*f+lower.ra;
  rv.ra=fmod(rv.ra,24.0);
  rv.dec=(upper.dec-lower.dec)*f+lower.dec;
  return rv;
}



  
