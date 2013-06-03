#include <cmath>
#include <complex>
using std::complex;
#include <vector>
#include <list>

#include "seti_config.h"
#ifndef _DB_TABLE_H_
#include "db_table.h"
#include "sqlapi.h"
#include "schema_master.h"
#else
class spike;
class autocorr;
class gaussian;
class pulse;
class triplet;
class spike_small;
class autocorr_small;
class gaussian_small;
class pulse_small;
class triplet_small;
#endif
#include "seti_byteorder.h"
#include "seti_tel.h"
#include "seti_cfg.h"
#include "seti_time.h"
#include "seti_coord.h"
#include "seti_healpix.h"
#include "seti_doppler.h"
#include "seti_aoutils.h"
#include "least_squares.h"
#include "smooth.h"
#include "seti_dr2utils.h"
#include "seti_db_queries.h"
#include "seti_synthetic_data.h"
#include "truncated_add.h"
#include "seti_signal.h"
#include "seti_rfi.h"
#include "seti_rfi_null.h"
#include "seti_rfi_algo.h"
#include "seti_rfi_zone.h"
#include "seti_rfi_staff.h"
#include "seti_window_func.h"

#include "healpix_base.h"
extern "C" {
#include "chealpix.h"
}

// The following seems to be necessary for the zone rfi code.
// We should not really need it.
# define INT32_MAX      (2147483647)
# define __INT64_C(c)  c ## L
# define INT64_MAX      (__INT64_C(9223372036854775807))
#define isfinite(x)               \
 (sizeof (x) == sizeof (float)    \
 ? __finitef (x)                  \
 : sizeof (x) == sizeof (double)  \
 ? __finite (x) : __finitel (x))
