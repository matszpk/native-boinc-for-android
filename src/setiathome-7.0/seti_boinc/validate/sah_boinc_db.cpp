// stuff that refers to the BOINC DB.
// Separate to avoid name conflicts w/ SETI@home DB

#include <sys/types.h>
#include <dirent.h>

#include "parse.h"
#include "error_numbers.h"
#include "sah_boinc_db.h"
#include "sched_config.h"
#include "sched_util.h"
#include "util.h"
#include "validate_util.h"
#include "filesys.h"
#include "sched_msgs.h"


extern SCHED_CONFIG config;

int get_result_file(RESULT& r, SAH_RESULT& s) {
    int retval;
    FILE * f;
    char filename[256];
    char * path;
    std::string path_str;

    // Obtain and check the full path to the boinc result file.
    retval = get_output_file_path(r, path_str);
    if (retval) {
        if (retval == ERR_XML_PARSE) {
            log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                                "[%s] Cannot extract filename from canonical result %ld.\n",
                                r.name,  r.id);
            return(retval);
        } else {
            log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                                "[%s] unknown error from get_output_file_path() fpr result %ld.\n",
                                r.name,  r.id);
            return(retval);
        }
    } else {
        path = (char *)path_str.c_str();
        if (!boinc_file_exists(path)) {
            log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                                "[%s] Output file %s does not exist for result %ld\n",
                                r.name, path,  r.id);
            return(-1);
        }
    }

    retval = try_fopen(path, f, "r");
    if (retval) return retval;
    s.have_result = true;   // we had good read of the result file
    s.parse_file(f);
    fclose(f);
    return 0;
}
