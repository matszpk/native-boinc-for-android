#include "sah_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <list>
#include <time.h>
#include <math.h>


#include "boinc_db.h"
#include "sched_config.h"
#include "sah_assimilate_handler.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "parse.h"
#include "sched_util.h"
#include "util.h"
#include "validate_util.h"
#include "filesys.h"

#include "setilib.h"

#include "timecvt.h"
#include "db_table.h"
#include "schema_master.h"
#include "app_config.h"
#include "sqlint8.h"
#include "sah_result.h"

// the boinc assimilator makes these available
extern SCHED_CONFIG config;
extern bool noinsert;		

const double  D2R =  0.017453292;
const int fpix_res = 10;            // Hz

const float bad_fp_val_marker = -91.0;

struct timespec nanotime;

//inline long round(double x) {return long(floor(x + 0.5f));}

int populate_seti_result(
	result& sah_result, 
        RESULT& boinc_canonical_result, 
        WORKUNIT& b_wu,
        long long & seti_wu_id 
);
template<typename T> int insert_signals( T&                  signal,
                                         char *              signal_name,
                                         char *              wu_name,
                                         sqlint8_t           sah_result_id,
                                         std::ifstream&      result_file,
                                         receiver_config&    receiver_cfg,
                                         int                 appid,
                                         int                 max_signals_allowed,
                                         bool                check_rfi,
                                         list<long>&         hotpix);
template<typename T> int pre_process(T& signal, receiver_config& receiver_cfg, sqlint8_t sah_result_id, char * wu_name, bool check_rfi);
template<typename T> int check_values(T& signal, sqlint8_t sah_result_id, char * wu_name);
int get_science_configs(WORKUNIT& boinc_wu, long long seti_wu_id, receiver_config& receiver_cfg, analysis_config& analysis_cfg);
int parse_settings_id(WORKUNIT& boinc_wu);
int get_configs_old_method(WORKUNIT& boinc_wu, long long seti_wu_id, receiver_config& receiver_cfg, analysis_config& analysis_cfg);
long long new_wu_id(long long old_wu_id);


// assimilate_handler() is called by BOINC code and is passed the canonical 
// result for a workunit.  assimilate_handler() reads the referenced result
// file and inserts the result and its signals into the master science DB.
// BOINC also passes the workunit (as it appears in the BOINC DB) and a vector
// containing all results (including the canonical one) for that workunit.
// We use the workunit to determine if there is an error condition.
int assimilate_handler(
    WORKUNIT& boinc_wu, vector<RESULT>& boinc_results, RESULT& boinc_canonical_result
) {
    int retval=0;
    int spike_count=0, spike_inserted_count=0, gaussian_count=0, gaussian_inserted_count=0, pulse_count=0, pulse_inserted_count=0, triplet_count=0, triplet_inserted_count=0, autocorr_count=0, autocorr_inserted_count=0;
    static receiver_config receiver_cfg;
    static analysis_config analysis_cfg;
    workunit s_wu;
    workunit_grp s_wu_grp;
    result sah_result;
    spike sah_spike;
    autocorr sah_autocorr;
    gaussian sah_gaussian;
    pulse sah_pulse;
    triplet sah_triplet;
    char filename[256];
    char * path;
    std::string path_str;
    long sah_result_id;
    sqlint8_t sah_spike_id, sah_autocorr_id, sah_gaussian_id, sah_pulse_id, sah_triplet_id;
    static bool first_time = true;
    int sql_error_code;
    long long seti_wu_id;
    time_t now;
    int hotpix_update_count;
    int hotpix_insert_count;

    APP_CONFIG sah_config;

    hotpix hotpix;
    list<long> qpixlist;            // will be a unique list of qpixes for
                                    // updating the hotpix table
    list<long>::iterator qpix_i;

    nanotime.tv_sec = 0;
    nanotime.tv_nsec = 1000000;


    // app specific configuration
    if (first_time) {
	first_time = false;
	receiver_cfg.id = 0;
	analysis_cfg.id   = 0;
    	retval = sah_config.parse_file("..");
    	if (retval) {
      		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
       	     	"First entrance to handler : can't parse config file. Exiting.\n"
      		);
  		return(retval);
	} else {
		retval = db_change(sah_config.scidb_name); 
		if (!retval) {
      			log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
       	     		"First entrance to handler : could not open science DB %s. Exiting.\n",
			sah_config.scidb_name
      			);
  			return(retval);
		} else {
      			log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
       	     		"First entrance to handler : using science DB %s\n",
			sah_config.scidb_name
      			);
		}
        log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
       	 	"Will%s check signals for rfi\n", 
            sah_config.assim_check_rfi ? "" : " not"
    	);
	}
    	// Sometimes we want to perform all assimilation functions
    	// *except* insertion into the science master DB.
    	if (noinsert) {
      		log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
      		"[%s] assimilator is in noinsert mode.\n",
      		boinc_wu.name
      		);
    	}
    } else {
/*
	retval = db_change(sah_config.scidb_name);
        if (!retval) {
              log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                  "First entrance to handler : could not open science DB %s. Exiting.\n",
                  sah_config.scidb_name
              );
              return(retval);
        } else {
              log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
                  "First entrance to handler : using science DB %s\n",
                  sah_config.scidb_name
              );
        }
*/
   }
   if (noinsert) return 0;   	// Note that this will result in the WU being marked as assimilated - 
				// we will not see it again.

    // translate seti wuid for thos wus that changed ids during the DB merge
    seti_wu_id = new_wu_id((long long)boinc_wu.opaque);
   log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
         	"[%s] old seti WU id is : %lld  new seti WU id is : %lld\n", 
            boinc_wu.name, (long long)boinc_wu.opaque, seti_wu_id
   );

    if (boinc_wu.canonical_resultid) {
      log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
            "[%s] Canonical result is %d.  SETI workunit ID is %lld.\n", 
	    boinc_wu.name,  boinc_wu.canonical_resultid, seti_wu_id
      );
    } else {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
            "[%s] No canonical result\n", boinc_wu.name
      );
    }

    if (!boinc_wu.canonical_resultid) {
	return 0;		// Note that this will result in the WU being marked as assimilated - 
                                // we will not see it again.  No canonical result means that
				// too many results were returned with no concensus. 

    }

    // Obtain and check the full path to the boinc result file.
    retval = get_output_file_path(boinc_canonical_result, path_str);
    if (retval) {
	if (retval == ERR_XML_PARSE) {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
			"[%s] Cannot extract filename from canonical result %ld.\n",
            		boinc_wu.name,  boinc_wu.canonical_resultid);
        	return(retval);
	} else {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                        "[%s] unknown error from get_output_file_path() for result %ld.\n",
                        boinc_wu.name,  boinc_wu.canonical_resultid);
                return(retval);
   	}
     } else {
     	path = (char *)path_str.c_str();
     	if (!boinc_file_exists(path)) {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        		"[%s] Output file %s does not exist for result %ld.\n",
               	  	boinc_wu.name, path,  boinc_wu.canonical_resultid);
        	return(-1);
     	} else {
		log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
                        "[%s] Result %ld : using upload file %s\n",
               	  	boinc_wu.name, boinc_wu.canonical_resultid, path);
	}
    }

    // Open it.
    std::ifstream result_file(path, ios_base::in);
    if (!result_file.is_open()) {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
           "[%s] open error for result file %s : errno %d\n", 
           boinc_wu.name, path, errno
      );
      return -1;
    }

    retval = get_science_configs(boinc_wu, seti_wu_id, receiver_cfg, analysis_cfg);
    if (retval) {
	if (retval == 100) {
		return (0);
	} else {
		return (-1);
 	}
    }
    log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
                        "[%s] Result %ld : using receiver_cfg %d and analysis_cfg %d\n",
               	  	boinc_wu.name, boinc_wu.canonical_resultid, receiver_cfg.id, analysis_cfg.id);

    // Insert a sah result
    retval = populate_seti_result(sah_result, boinc_canonical_result, boinc_wu, seti_wu_id);
    sah_result_id = sah_result.insert();
    if (sah_result_id) {
    	log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
         		"[%s] Inserted result.  Boinc result id is %d.  Sah result id is %lld.\n", 
	 		boinc_wu.name, boinc_canonical_result.id, 
			(long long)sah_result_id
   	);
    } else {
	if (sql_last_error_code() == -239 || sql_last_error_code() == -268) {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                        "[%s] Could not insert duplicate result.  SQLCODE is %ld.  SQLMSG is %s.\n",
                        boinc_wu.name, sql_last_error_code(), sql_error_message()
        	);
		return 0; 	// non-fatal - we will never see this result again
	} else {
		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                        "[%s] Could not insert result.  SQLCODE is %ld.  SQLMSG is %s.\n",
                        boinc_wu.name, sql_last_error_code(), sql_error_message()
        	);
        	return -1;	// fatal - non-dup error
	}
    }

    // Insert all sah signals in turn
    insert_signals( sah_spike, 
                    "spike", 
                    boinc_wu.name, 
                    sah_result_id,
                    result_file, 
                    receiver_cfg, 
                    boinc_canonical_result.appid, 
                    analysis_cfg.max_spikes,
                    sah_config.assim_check_rfi,
                    qpixlist);

    insert_signals( sah_autocorr, 
                    "autocorr", 
                    boinc_wu.name, 
                    sah_result_id,
                    result_file, 
                    receiver_cfg, 
                    boinc_canonical_result.appid, 
                    analysis_cfg.max_autocorr,
                    sah_config.assim_check_rfi,
                    qpixlist);

    insert_signals( sah_gaussian, 
                    "gaussian", 
                    boinc_wu.name, 
                    sah_result_id,
                    result_file, 
                    receiver_cfg, 
                    boinc_canonical_result.appid, 
                    analysis_cfg.max_gaussians,
                    sah_config.assim_check_rfi,
                    qpixlist);

    insert_signals( sah_pulse, 
                    "pulse", 
                    boinc_wu.name, 
                    sah_result_id,
                    result_file, 
                    receiver_cfg, 
                    boinc_canonical_result.appid, 
                    analysis_cfg.max_pulses,
                    sah_config.assim_check_rfi,
                    qpixlist);

    insert_signals( sah_triplet, 
                    "triplet", 
                    boinc_wu.name, 
                    sah_result_id,
                    result_file, 
                    receiver_cfg, 
                    boinc_canonical_result.appid, 
                    analysis_cfg.max_triplets,
                    sah_config.assim_check_rfi,
                    qpixlist);

    // update last hit time to now for each qpix hit
    qpixlist.unique();
    hotpix_update_count = 0;
    hotpix_insert_count = 0;
    qpix_set_hotpix(qpixlist, hotpix_update_count, hotpix_insert_count);
//#define DEBUG_NEIGHBORS
#ifdef DEBUG_NEIGHBORS
    for(qpix_i = qpixlist.begin(); qpix_i != qpixlist.end(); qpix_i++) fprintf(stderr, "%ld\n", *qpix_i);
#endif
    qpixlist.clear();
    log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
             "[%s] Updated %d rows and inserted %d rows in the hotpix table\n",
             boinc_wu.name, hotpix_update_count, hotpix_insert_count
    );

    return 0;   // the successful assimilation of one WU
}


template<typename T>
int insert_signals( T&                  signal,   
                    char *              signal_name, 
                    char *              wu_name,
                    sqlint8_t           sah_result_id,
                    std::ifstream&      result_file, 
                    receiver_config&    receiver_cfg,   
                    int                 appid,
                    int                 max_signals_allowed,
                    bool                check_rfi,
                    list<long>&         qpixlist) {

    int signal_count=0, signal_inserted_count=0, retval=0, qpix;
    sqlint8_t signal_id=0;

    result_file.clear();
    result_file.seekg(0);
    while (!result_file.eof()) {
        result_file >> signal;
        if (!result_file.eof()) {
            signal_count++;
            signal.result_id = sah_result_id;      // fill in any additional signal fields 
            if (max_signals_allowed == 0 || signal_count <= max_signals_allowed) {
                if (!(signal.rfi_found = check_values(signal, sah_result_id, wu_name))) {
                    // preprocess only if we have good values
                    retval = pre_process(signal, receiver_cfg, sah_result_id, wu_name, check_rfi);  
                    qpixlist.push_back(npix2qpix((long long)signal.q_pix));
                }
                signal_id = signal.insert();
                if (signal_id) {
                    log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
                        "[%s] Inserted %s %"INT8_FMT" for sah result %"INT8_FMT"\n",
                        wu_name, signal_name, INT8_PRINT_CAST(signal_id), INT8_PRINT_CAST(sah_result_id)
                    );
                    signal_inserted_count++;
                } else {
                    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                                "[%s] Could not insert %s for sah result %"INT8_FMT". SQLCODE is %d. q_pix is %"INT8_FMT"  ra is %lf  decl is %lf .\n",
                                wu_name, signal_name, sah_result_id, sql_last_error_code(), signal.q_pix, signal.ra, signal.decl
                    );
#if 0
                    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                                "[%s] Could not insert %s for sah result %ld. SQLCODE is %d.  SQLMSG is %s  q_pix is %"INT8_FMT".\n",
                                wu_name, signal_name, sah_result_id, sql_last_error_code(), sql_error_message(), signal.q_pix
                    );
#endif
                    return -1;
                }  
            }   
        }   
    } 

    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
        "[%s] Inserted %d out of %d %s(s) for sah result %"INT8_FMT" \n",
        wu_name, signal_inserted_count, signal_count, signal_name, INT8_PRINT_CAST(sah_result_id)
    );

}


int populate_seti_result(
	result& s_result, RESULT& b_result, WORKUNIT& b_wu, long long & seti_wu_id
) {

    // from boinc result
    s_result.boinc_result	= b_result.id;
    s_result.received		= time_t_to_jd((time_t) b_result.received_time);
    s_result.wuid		    = seti_wu_id;
    s_result.hostid		    = b_result.hostid;
    s_result.versionid		= b_result.app_version_num;
    s_result.return_code	= b_result.exit_status;
    if ((int)b_result.opaque & RESULT_FLAG_OVERFLOW) s_result.overflow = 1;

    // from boinc WU
    s_result.reserved		= b_wu.error_mask;

    return(0);
	
}

template<typename T>
int pre_process(T& signal, receiver_config& receiver_cfg, sqlint8_t sah_result_id, char * wu_name, bool check_rfi) {

    //long q_pix;

    // barycenter reference frame - note that we pass epoch of the 
    // day coordinates as seti_dop_FreqAtBaryCenter() does precession.
    // We should change this but such a change will involve several
    // projects.
    signal.barycentric_freq =  seti_dop_FreqAtBaryCenter(signal.detection_freq,
                                                         signal.time,
                                                         signal.ra,
                                                         signal.decl,
                                                         stdepoch,
                                                         (telescope_id)receiver_cfg.s4_id
                                                        );

    // precess in place
    eod2stdepoch(signal.time, signal.ra, signal.decl, stdepoch);

    // cubic pixel number with precessed coords plus course frequency
    signal.q_pix = co_radeclfreq2npix(signal.ra, signal.decl, signal.barycentric_freq);

    //co_radecl2pix(signal.ra, signal.decl, q_pix);
    // move the sky portion to the high order 4 bytes 
    // and add in freq portion (will land in the low order 4 bytes). 
    //signal.q_pix = q_pix;
    //signal.q_pix = (signal.q_pix << 32) + ((unsigned long)round(signal.barycentric_freq / fpix_res));  

    // send the signal through the rfi checker if configured to do so
    if(check_rfi) {
        if (is_rfi(signal)) {
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
                                "[%s] Signal for result %"INT8_FMT" flagged as RFI\n",
                                wu_name, sah_result_id
            );
        } else {
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
                                "[%s] Signal for result %"INT8_FMT" flagged as RFI CLEAN\n",
                                wu_name, sah_result_id
            );
        }
    }

    return 0;
}
 
template<typename T>
int check_values(T& signal, sqlint8_t sah_result_id, char * wu_name) {

    int retval=0;

    // check for ranges and NaN's

    if(signal.ra < 0 || signal.ra >= 24 || signal.ra != signal.ra) {
   	    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
       	                    "[%s] Signal for result %"INT8_FMT" has out of range RA : %lf\n", 
	                        wu_name, sah_result_id, signal.ra
    	);
	    signal.ra = bad_fp_val_marker;
    	retval = 1;
    }
    if(signal.decl < -90 || signal.decl > 90 || signal.decl != signal.decl) {
   	    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
       	                    "[%s] Signal for result %"INT8_FMT" has out of range declination : %lf\n", 
	                        wu_name, sah_result_id, signal.decl
    	);
	    signal.decl = bad_fp_val_marker;
    	retval = 1;
    }
    // time range is from 1990 to 2050
    if(signal.time < 2447892.5 || signal.time > 2469807.5 || signal.time != signal.time) {
   	    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
       	                    "[%s] Signal for result %"INT8_FMT" has out of range time : %lf\n", 
	                        wu_name, sah_result_id, signal.time
    	);
	    signal.time = bad_fp_val_marker;
    	retval = 1;
    }
    if(signal.freq < 0 || signal.freq != signal.freq) {
   	    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
       	                    "[%s] Signal for result %"INT8_FMT" has out of range frequency : %lf\n", 
	                        wu_name, sah_result_id, signal.freq
    	);
	    signal.freq = bad_fp_val_marker;
    	retval = 1;
    }

    return(retval);
}

int get_science_configs(WORKUNIT& boinc_wu, long long seti_wu_id, receiver_config& receiver_cfg, analysis_config& analysis_cfg) {

    int settings_id, retval=0;
    static int first_time=1, current_settings_id=0, current_analysis_config_id=0, current_receiver_config_id=0;
    settings settings;

#if 1
    settings_id = parse_settings_id(boinc_wu);
    if (settings_id) {
        log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
             "[%s] Name string contains  settings id %ld\n",
             boinc_wu.name, settings_id
        );
    } else {
        log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,
             "[%s] Name string contains no settings id\n",
             boinc_wu.name
        );
    }
#endif

#if 0
    // temp hack
    if (first_time) {
	    retval = get_configs_old_method(boinc_wu, seti_wu_id, receiver_cfg, analysis_cfg);
	    if (!retval) first_time = 0;
    }
    return (retval);
    // end temp hack
#endif

    // can be removed after all WUs have settings (no settings would then be a fatal error)
    if (!settings_id) {
	    get_configs_old_method(boinc_wu, seti_wu_id, receiver_cfg, analysis_cfg);
        return(0);
    }
    // end can be removed after all WUs have settings

    if (settings_id == current_settings_id) {
	    return (0);
    } else {
        current_settings_id = settings_id;

	    // get settings row
	    if (settings.fetch(settings_id)) {
            log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
                    "[%s] Obtained science master DB row for settings table id %ld\n",
                    boinc_wu.name, settings.id
            );
        } else {
            log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                    "[%s] Could not obtain science master DB row for settings table id %ld. SQLCODE is %d\n",
                    boinc_wu.name, settings_id, sql_last_error_code()
            );
		    return(-1);
	    }

	    // get analysis config row
	    if (settings.analysis_cfg.id != current_analysis_config_id) {
		    if (analysis_cfg.fetch(settings.analysis_cfg.id)) {
                current_analysis_config_id = analysis_cfg.id;
			    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
                       		"[%s] Obtained science master DB row for analysis_config table id %ld\n",
                     		boinc_wu.name, analysis_cfg.id
                );
            } else {
               	log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                        	"[%s] Could not obtain science master DB row for analysis_config table id %ld. SQLCODE is %d\n",
                        	boinc_wu.name, settings.analysis_cfg.id, sql_last_error_code()
               	);
			    return(-1);
		    }
        }

	    // get receiver config row 
	    if (settings.receiver_cfg.id != current_receiver_config_id) {
		    if (receiver_cfg.fetch(settings.receiver_cfg.id)) {
                current_receiver_config_id = receiver_cfg.id;
			    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
                       		"[%s] Obtained science master DB row for receiver_config table id %ld\n",
                       		boinc_wu.name, receiver_cfg.id
                );
           	} else {
               	log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                           	"[%s] Could not obtain science master DB row for receiver_config table id %ld. SQLCODE is %d\n",
                        	boinc_wu.name, settings.receiver_cfg.id, sql_last_error_code()
               	);
			    return(-1);
		    }
        }
        
        return(0);

    }
}


int parse_settings_id(WORKUNIT& boinc_wu) {

    int i=0, j=0, num_tokens=1, pos=0, len=0;
    const int max_tokens=7, max_token_len=128, settings_token=4;
    char tokens[max_tokens][max_token_len];

    len = strlen(boinc_wu.name);

    // parse the dot delimited WU name into its
    // constituent tokens
    for (pos=0, i=0, j=0; 
         pos < len && i < max_tokens; 
         pos++) {
        if (boinc_wu.name[pos] == '.') {
                // terminate this token
                tokens[i][j] = '\0';
                j = 0;
                i++;
                num_tokens++;
                continue;
        }
        if (j >= max_token_len) {
      		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        		"[%s] Token too long during name parsing.\n",
        		boinc_wu.name
      		);
            exit(1);
        } else {
            // continue this token
            tokens[i][j] = boinc_wu.name[pos];
            j++;
        }
    }

    if (i >= max_tokens) {
	log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
                "[%s] Too many tokens during name parsing.\n",
                boinc_wu.name
        );
        exit(1);
    } else {
        tokens[i][j] = '\0';		// terminate the final token
        num_tokens++;
    }

    if (i >= settings_token-1) {		// does name cointain settings?
        return(atoi(tokens[settings_token]));    // yes - return it
    } else {
	    return(0);
    }
}


int get_configs_old_method(WORKUNIT& boinc_wu, long long seti_wu_id, receiver_config& receiver_cfg, analysis_config& analysis_cfg) {

    workunit 	  s_wu;
    workunit_grp  s_wu_grp;
    int 	  sql_error_code;

    // workunit
    if (s_wu.fetch(seti_wu_id)) {
      log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
        "[%s] Obtained science master DB row for wu %lld [%s]\n",
        boinc_wu.name, seti_wu_id, s_wu.name
      );
    } else {
      sql_error_code = sql_last_error_code();
      if (sql_error_code == 100) {
      	log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        	"[%s] Science master DB row for wuid %lld does not exit.  Marking as assimilated.\n",
        	boinc_wu.name, seti_wu_id
      	);
	return sql_error_code;
      } else {	
      	log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        	"[%s] Could not obtain science master DB row for wuid %lld. SQLCODE is %d\n",
        	boinc_wu.name, seti_wu_id, sql_error_code
      	);
        return -1;
      }
    }

    // workunit_grp
    if (s_wu_grp.fetch(s_wu.group_info.id)) {
      log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
        "[%s] Obtained science master DB row for wugid %ld\n",
        boinc_wu.name, s_wu_grp.id
      );
    } else {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        "[%s] Could not obtain science master DB row for wugid %ld. SQLCODE is %d\n",
        boinc_wu.name, s_wu.group_info.id, sql_last_error_code() 
      );
      return -1;
    }

    // receiver_config - get it if it has changed
    if (receiver_cfg.id != s_wu_grp.receiver_cfg.id) {
    	if (receiver_cfg.fetch(s_wu_grp.receiver_cfg.id)) { 
      		log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
        		"[%s] Obtained science master DB row for receiver table id %ld [%s]\n",
        		boinc_wu.name, receiver_cfg.id, receiver_cfg.name
      		);
    	} else {
      		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        		"[%s] Could not obtain science master DB row for receiver table id %ld. SQLCODE is %d\n",
        		boinc_wu.name, s_wu_grp.receiver_cfg.id, sql_last_error_code() 
      		);
      	return -1;  
    	}
    }

    // analysis_config - get it if it has changed
    if (analysis_cfg.id != s_wu_grp.analysis_cfg.id) {
     	if (analysis_cfg.fetch(s_wu_grp.analysis_cfg.id)) { 
      		log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,
       		 	"[%s] Obtained science master DB row for analysis table id %ld.\n",
       		 	boinc_wu.name, analysis_cfg.id
      		);
    	} else {
      		log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,
        		"[%s] Could not obtain science master DB row for analysis table id %ld. SQLCODE is %d\n",
        		boinc_wu.name, s_wu_grp.analysis_cfg.id, sql_last_error_code()
      		);
      	return -1;
    	}
    }

    return 0;
}

long long new_wu_id(long long old_wu_id) {

    workunit seti_wu;
    char buf[256];

    sprintf(buf,"where sb_id=%lld", old_wu_id);

    if (!seti_wu.fetch(std::string(buf))) {
        return (old_wu_id);
    } else {
        return (seti_wu.id);
    }
    
}
