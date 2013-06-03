/*
 *
 * Functions for managing wufiles and their data
 *
 * $Id: mb_wufiles.cpp,v 1.1.2.6 2007/08/10 18:21:13 korpela Exp $
 *
 */

#include "sah_config.h"
#undef USE_MYSQL
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "boinc_db.h"
#include "sched_util.h"
#include "setilib.h"
#include "splitparms.h"
#include "splittypes.h"
#include "s_util.h"
#include "util.h"
#include "str_util.h"
#include "str_replace.h"
#include "mb_splitter.h"
#include "message.h"
#include "mb_angdist.h"
#include "cmap_interp.h"
#include "lcgamm.h"
#include "sqlrow.h"
#include "sqlblob.h"
#include "sqlapi.h"
#include "db_table.h"
#include "schema_master.h"
#include "xml_util.h"
#include "db/app_config.h"
#include "str_util.h"

std::vector<long long> wu_database_id;
std::vector<std::vector<unsigned char> > bin_data;

extern APP_CONFIG sah_config;

int make_wu_headers(std::vector<dr2_compact_block_t> &tapebuffer, telescope_id
tel, std::vector<workunit> &wuheader) {
  int procid=getpid();
  double receiver_freq;
  int bandno;
  FILE *tmpfile;
  char tmpstr[256];
  char buf[64];
  static const receiver_config &r(rcvr);
  static const settings &s(splitter_settings);
  bool group_is_vlar;

  if (!strncmp(s.splitter_cfg->data_type,"encoded",
     std::min(static_cast<size_t>(7),sizeof(s.splitter_cfg->data_type)))) {
    noencode=0;
  } else {
      noencode=1;
  }

  tapebuffer[0].header.samplerate*=1e+6;
  seconds sample_time(1.0/tapebuffer[0].header.samplerate);
  seti_time start_time(tapebuffer[0].header.data_time
           -tapebuffer[0].data.size()*0.5*sample_time);
  seti_time end_time(tapebuffer[tapebuffer.size()-1].header.data_time);


  workunit_grp wugrp;
  sprintf(wugrp.name,"%s.%ld.%d.%d.%d",
    tapebuffer[0].header.name,
    procid, 
    tapebuffer[0].header.dataseq,
    tel-AO_430,
    s.id);
  wugrp.receiver_cfg=r; 
  wugrp.recorder_cfg=s.recorder_cfg; 
  wugrp.splitter_cfg=s.splitter_cfg; 
  wugrp.analysis_cfg=s.analysis_cfg; 

  wugrp.data_desc.nsamples=NSAMPLES;
  wugrp.data_desc.true_angle_range=0;
  coordinate_t start_coord(cmap_interp(coord_history,start_time));
  coordinate_t end_coord(cmap_interp(coord_history,end_time));
  wugrp.data_desc.start_ra=start_coord.ra;
  wugrp.data_desc.end_ra=end_coord.ra;
  wugrp.data_desc.start_dec=start_coord.dec;
  wugrp.data_desc.end_dec=end_coord.dec;
  coordinate_t last_coord=start_coord;
  double sample_rate=tapebuffer[0].header.samplerate/NSTRIPS;

  // find the bracketing entries in the coordinate history
  std::map<seti_time,coordinate_t>::iterator above(coord_history.upper_bound(end_time));
  std::map<seti_time,coordinate_t>::iterator below(coord_history.lower_bound(start_time));
  std::map<seti_time,coordinate_t>::iterator p;
  if (above==coord_history.begin()) {
    above++;
  }
  if (below==coord_history.end()) {
    below=above;
    below--;
  }
  if (above==below) {
    below--;
  }
  // Calculate the angular distance the beam has traveled
  for (p=below;p!=above;p++)
  {
    wugrp.data_desc.true_angle_range+=angdist(last_coord,p->second);
    last_coord=p->second;
  }
  wugrp.data_desc.true_angle_range+=angdist(last_coord,end_coord);
  if (wugrp.data_desc.true_angle_range==0) wugrp.data_desc.true_angle_range=1e-10;
  // Calculate the number of unique signals that could be found in a workunit.
  // We will use these numbers to calculate thresholds.
  double numgauss=2.36368e+08/std::min(wugrp.data_desc.true_angle_range,10.0);
  double numpulse=std::min(4.52067e+10/std::min(wugrp.data_desc.true_angle_range,10.0),2.00382e+11);
  double numtrip=std::min(3.25215e+12/std::min(wugrp.data_desc.true_angle_range,10.0),1.44774e+13);

  // check for VLAR workunits
  if (wugrp.data_desc.true_angle_range < 0.12) {
    group_is_vlar=true;
  } else {
    group_is_vlar=false;
  }
    

  // if (useanalysiscfgid > 0) { 
  //   log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Re-reading analysis cfg id: %d (set by user):\n",useanalysiscfgid);
  //   s.analysis_cfg = useanalysiscfgid;
  //   }

  // Calculate a unique key to describe this analysis config.
  long keyuniq=floor(std::min(wugrp.data_desc.true_angle_range*100,1000.0)+0.5)+
    s.analysis_cfg.id*1024;
  if ((keyuniq>((s.analysis_cfg.id+1)*1024)) ||(keyuniq<(s.analysis_cfg.id)*1024)) {
     log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Invalid keyuniq value!\n");
     log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"%d %d %f\n",keyuniq,s.analysis_cfg.id,wugrp.data_desc.true_angle_range);
     exit(1);
  }

  keyuniq*=-1;
  long save_keyuniq=keyuniq;
  splitter_settings.analysis_cfg=wugrp.analysis_cfg;
  sprintf(tmpstr,"where keyuniq=%d",keyuniq); 
  // Check if we've already done this analysis_config...
  // Fetch through splitter_settings, since it's alias (s) is const.
  splitter_settings.analysis_cfg.id=0;
  splitter_settings.analysis_cfg->fetch(tmpstr);

  if (s.analysis_cfg->id==0) {
    if (keyuniq != save_keyuniq) {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"keyuniq value changed!\n");
      exit(1);
    }

    // If not calculate the thresholds based upon the input analysis_config
    // Triplets are distributed exponentially...
    wugrp.analysis_cfg->triplet_thresh+=(log(numtrip)-29.0652);

    // Gaussians are based upon chisqr...
    double p_gauss=lcgf(32.0,wugrp.analysis_cfg->gauss_null_chi_sq_thresh*32.0);
    p_gauss-=(log(numgauss)-19.5358);
    wugrp.analysis_cfg->gauss_null_chi_sq_thresh=invert_lcgf(p_gauss,32,1e-4)*0.03125;

    // Pulses thresholds are log of the probability
    wugrp.analysis_cfg->pulse_thresh+=(log(numpulse)-24.7894);

    wugrp.analysis_cfg->keyuniq=keyuniq;
    wugrp.analysis_cfg->insert();
  } else {
    wugrp.analysis_cfg=s.analysis_cfg;
  }

  strlcpy(wugrp.data_desc.time_recorded,
      short_jd_string(start_time.jd().uval()),
      sizeof(wugrp.data_desc.time_recorded));
  wugrp.data_desc.time_recorded_jd=start_time.jd().uval();
  wugrp.data_desc.coords.clear();

  wugrp.data_desc.coords.push_back(start_coord);  
  for (p=below;p!=above;p++)
  {
     wugrp.data_desc.coords.push_back(p->second);  
  }
  wugrp.data_desc.coords.push_back(end_coord);

  wugrp.tape_info->id=0;
  sprintf(buf,"%d",tel-AO_ALFA_0_0);
  wugrp.tape_info->fetch(std::string("where name=\'")+tapebuffer[0].header.name+"\' and beam="+buf);
  wugrp.tape_info->start_time=tapebuffer[0].header.data_time.jd().uval();
  wugrp.tape_info->last_block_time=wugrp.tape_info->start_time;
  wugrp.tape_info->last_block_done=tapebuffer[0].header.dataseq;
  wugrp.tape_info->beam=tel-AO_ALFA_0_0;

  if (!nodb) {
    if (wugrp.tape_info.id) {
      if (!(wugrp.tape_info->update())) {
        char buf[1024];
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"%s",sql_error_message());
	exit(1);
      }
    } else {
      strlcpy(wugrp.tape_info->name,tapebuffer[0].header.name,sizeof(wugrp.tape_info->name));
      wugrp.tape_info->insert();
    }
  }

  if (!nodb) {
    sqlint8_t wgid;
    if ((wgid=wugrp.insert())<=0) {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Workunit_grp insert failed\nwgid=%d\nSQLCODE=%d\nLAST_NON_ZERO_SQLCODE=%d\n",wgid,sql_error_code(),sql_last_error_code());
      exit( 1 );
    }
    wugrp.id=wgid;
  }
  int i; 
  wu_database_id.resize(NSTRIPS);
  bin_data.resize(NSTRIPS);
  wuheader.resize(NSTRIPS);
  for (i=0;i<NSTRIPS;i++) {
    bin_data[i].clear();
    wuheader[i].group_info=wugrp;
    wuheader[i].group_info.id=wugrp.id;
    sprintf(wuheader[i].name,"%s.%ld.%d.%ld.%d.%d",tapebuffer[0].header.name,
       procid, tapebuffer[0].header.dataseq,
       tel-AO_430,s.id,i);
    if (group_is_vlar) {
      strlcat(wuheader[i].name,".vlar",sizeof(wuheader[i].name));
    }
    wuheader[i].subband_desc.sample_rate=tapebuffer[0].header.samplerate/NSTRIPS;
 
    receiver_freq=tapebuffer[0].header.sky_freq;

    bandno=((i+NSTRIPS/2)%NSTRIPS)-NSTRIPS/2;

    wuheader[i].subband_desc.base=receiver_freq+
	       (double)(bandno)*wuheader[i].subband_desc.sample_rate;
    wuheader[i].subband_desc.center=receiver_freq+wuheader[i].subband_desc.sample_rate*NSTRIPS*((double)IFFT_LEN*bandno/FFT_LEN+(double)IFFT_LEN/(2*FFT_LEN)-1.0/(2*FFT_LEN));
    wuheader[i].subband_desc.number=i;

    if (!nodb ) {
      if (!(wu_database_id[i]=wuheader[i].id=wuheader[i].insert())) {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Database error in make_wu_headers()\n");
        exit(EXIT_FAILURE);
      }
    }
	
    sprintf(tmpstr,"./wu_inbox/%s",wuheader[i].name);
    if ((tmpfile=fopen(tmpstr,"w"))) {
      fprintf(tmpfile,"<workunit>\n");
      fprintf(tmpfile,wuheader[i].print_xml().c_str());
      fclose(tmpfile);
    } else {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to open file ./wu_inbox/%s, errno=%d\n",wuheader[i].name,errno);
      exit(1);
    }
    bin_data[i].reserve(wuheaders[i].group_info->recorder_cfg->bits_per_sample*
       wuheaders[i].group_info->data_desc.nsamples/8);
  }
  return(1);
}


void write_wufile_blocks(int nbytes) {
  // doesn't do anything anymore.  What was done here is done in output_samples.
}

int filecopy(char *oldname,char *newname) {
  FILE *oldfile,*newfile;
  char buffer[16384];
  int nread;
  if ((oldfile=fopen(oldname,"rb")) && (newfile=fopen(newname,"wb"))) {
      do {
	nread=fread(buffer,1,16384,oldfile);
	fwrite(buffer,1,nread,newfile);
      } while (nread>0);
      fclose(oldfile);
      fclose(newfile);
      return 0;
  } else {
    return 1;
  }
}
      


void rename_wu_files() {
  int i, retval;
  char oldname[256],newname[1024];
  unsigned long sz;
  DB_WORKUNIT db_wu;
  const char *name[1];
  char *wudir="./wu_inbox";
  xml_encoding encoding=(noencode?_binary:_x_setiathome);
  FILE *tmpfile;

  if (boinc_db.open(boinc_config.db_name,boinc_config.db_host,boinc_config.db_user,boinc_config.db_passwd)) {
    boinc_db.print_error("boinc_db.open");
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Fatal error in boinc_db.open\n");
    exit(1);
  }

  for (i=0;i<NSTRIPS;i++) {
    name[0]=wuheaders[i].name;
    sprintf(oldname,"%s/%s",wudir,name[0]);

    //sprintf(newname,"%s%s/%s",projectdir,WU_SUBDIR,name[0]);
    retval = dir_hier_path(name[0],
                           boinc_config.download_dir,
                           boinc_config.uldl_dir_fanout,
                           newname,
			   true
    );
    if (retval) {
        char buf[1024];
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"[%s] dir_hier_path() failed: %d\n", name[0], retval);
	exit(1);
    } 

    struct stat sbuf;
    if (!stat(oldname,&sbuf) && (tmpfile=fopen(oldname,"a"))) {
       std::string tmpstr=xml_encode_string(bin_data[i],encoding);
       fprintf(tmpfile,"<data length=%ld encoding=\"%s\">",tmpstr.size(),
            xml_encoding_names[encoding]);
       fwrite(tmpstr.c_str(),tmpstr.size(),1,tmpfile);
       fprintf(tmpfile,"</data>\n");
       fprintf(tmpfile,"</workunit>\n");
       sz=bin_data[i].size();

       fclose(tmpfile);
    } else {
        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Header file no longer exists! splitter start script may be failing\n ");
	exit(1);
    }  
    if (!nodb) {
      if (!filecopy(oldname,newname)) {
        db_wu.clear();
        db_wu.opaque=wuheaders[i].id;
        strncpy(db_wu.name,name[0],sizeof(db_wu.name)-2);
        db_wu.appid=app.id;
        //db_wu.rsc_fpops_est=2.79248e+13*6;
        //db_wu.rsc_fpops_bound=4.46797e+14*6;
	double beam=wuheaders[i].group_info->receiver_cfg->beam_width;
        double ar=wuheaders[i].group_info->data_desc.true_angle_range;
        double dur=(double)(wuheaders[i].group_info->data_desc.nsamples)/wuheaders[i].subband_desc.sample_rate;
        double min_slew=wuheaders[i].group_info->analysis_cfg->pot_min_slew;
        double max_slew=wuheaders[i].group_info->analysis_cfg->pot_max_slew;
	if ( ar <= beam ) {
	  db_wu.rsc_fpops_est=9.193e+13;
	} else if ( ar <= ( dur*min_slew ) ) {
          db_wu.rsc_fpops_est=5.962e+13+2.296e+12/ar;
        } else if ( ar <= ( dur*max_slew ) ) {
          db_wu.rsc_fpops_est=5.749e+13+1.476e+13/ar;
        } else {
          db_wu.rsc_fpops_est=3.535e+13;
        }
	db_wu.rsc_fpops_est*=(0.333/wuheaders[i].group_info->analysis_cfg->chirp_resolution);
        db_wu.rsc_fpops_bound=db_wu.rsc_fpops_est*10;
        db_wu.rsc_memory_bound=33554432;
        db_wu.rsc_disk_bound=33554432;
        // Our minimum is a 40 MFLOP machine
        db_wu.delay_bound=std::max(86400.0*7,db_wu.rsc_fpops_est/4e+7);
        db_wu.min_quorum=sah_config.min_quorum;
        db_wu.target_nresults=sah_config.target_nresults;
        db_wu.max_error_results=sah_config.max_error_results;
        db_wu.max_total_results=sah_config.max_total_results;
        db_wu.max_success_results=sah_config.max_success_results;
        strncpy(db_wu.app_name,appname,sizeof(db_wu.app_name)-2);
        if (create_work(db_wu,
                        wu_template,
                        result_template_filename,
                        result_template_filepath,
                        name,
                        1,
		        boinc_config,
		        NULL
                       )
           ) {
          log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"create work failed\n");
          exit(1);
        }
        //unlink(oldname);   // we now *always* unlink
      } else {
	  log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"file copy failed\n");
          exit(1);
      }
      unlink(oldname);
    }
  }
  boinc_db.close();
}

/*
 * $Log: mb_wufiles.cpp,v $
 * Revision 1.1.2.6  2007/08/10 18:21:13  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.5  2007/08/09 21:31:08  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.4  2007/06/07 20:01:52  mattl
 * *** empty log message ***
 *
 * Revision 1.1.2.3  2007/06/06 15:58:30  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.2  2007/04/25 17:27:31  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.1  2006/12/14 22:24:45  korpela
 * *** empty log message ***
 *
 * Revision 1.29.2.14  2006/05/03 19:14:31  korpela
 * Updated work estimates.
 *
 * Revision 1.29.2.13  2006/04/24 18:41:02  korpela
 * *** empty log message ***
 *
 * Revision 1.29.2.12  2006/01/13 00:37:58  korpela
 * Moved splitter to using standard BOINC logging mechanisms.  All stderr now
 * goes to "error.log"
 *
 * Added command line parameters "-iterations=" (number of workunit groups to
 * create before exiting), "-trigger_file_path=" (path to the splitter_stop trigger
 * file.  Default is /disks/setifiler1/wutape/tapedir/splitter_stop).
 *
 * Reduced deadlines by a factor of three.  We now need a 30 MFLOP machine to meet
 * the deadline.
 *
 * Revision 1.29.2.11  2006/01/10 00:39:04  jeffc
 * *** empty log message ***
 *
 * Revision 1.29.2.10  2006/01/05 23:55:22  korpela
 * *** empty log message ***
 *
 * Revision 1.29.2.9  2005/12/05 22:11:40  korpela
 * Fixed bug in flops estimate.
 *
 * Revision 1.29.2.8  2005/10/05 16:22:17  jeffc
 * removed reference to old/new boolean for directory hash.
 *
 * Revision 1.29.2.7  2005/09/22 23:05:22  korpela
 * Fixed threshold calculation.  Was using chisqr rather than reduced chisqr.
 *
 * Revision 1.29.2.6  2005/09/21 22:11:23  korpela
 * Updated Makefile.in for OpenSSL.
 * Added dynamic threshold generation to wufiles.cpp.
 *
 * Revision 1.29.2.5  2005/08/01 17:47:38  korpela
 * Type fixed.
 *
 * Revision 1.29.2.4  2005/08/01 17:43:20  korpela
 * Refinement of FLOPS estimate for workunits.
 *
 * Revision 1.29.2.3  2005/07/26 17:17:01  korpela
 * Typo fix
 *
 * Revision 1.29.2.2  2005/07/19 00:15:19  korpela
 * Revised delay bound and FLOP estimate for setiathome_enhanced.
 *
 * Revision 1.29.2.1  2005/07/06 01:30:17  korpela
 * Updated estimates of FPOPS per workunit for setiathome enhanced.
 *
 * Revision 1.29  2005/03/08 22:36:15  jeffc
 * jeffc - fixed call to create_work()
 *
 * Revision 1.28  2005/02/15 23:06:47  korpela
 * Fixed missing dir_hier symbol.
 *
 * Revision 1.27  2004/12/27 20:48:54  jeffc
 * *** empty log message ***
 *
 * Revision 1.26  2004/11/18 22:24:48  korpela
 * *** empty log message ***
 *
 * Revision 1.25  2004/08/25 22:42:11  jeffc
 * *** empty log message ***
 *
 * Revision 1.24  2004/08/14 04:44:26  jeffc
 * *** empty log message ***
 *
 * Revision 1.23  2004/08/12 15:45:41  jeffc
 * *** empty log message ***
 *
 * Revision 1.22  2004/07/15 17:54:20  jeffc
 * *** empty log message ***
 *
 * Revision 1.21  2004/07/09 22:35:39  jeffc
 * *** empty log message ***
 *
 * Revision 1.20  2004/07/01 17:56:51  korpela
 * *** empty log message ***
 *
 * Revision 1.19  2004/06/25 13:49:33  jeffc
 * *** empty log message ***
 *
 * Revision 1.18  2004/06/18 23:23:32  jeffc
 * *** empty log message ***
 *
 * Revision 1.17  2004/06/16 20:57:19  jeffc
 * *** empty log message ***
 *
 * Revision 1.16  2004/06/02 20:51:32  jeffc
 * *** empty log message ***
 *
 * Revision 1.15  2004/01/22 00:57:54  korpela
 * *** empty log message ***
 *
 * Revision 1.14  2004/01/20 22:33:44  korpela
 * *** empty log message ***
 *
 * Revision 1.13  2004/01/06 22:44:05  korpela
 * *** empty log message ***
 *
 * Revision 1.12  2004/01/01 18:42:01  korpela
 * *** empty log message ***
 *
 * Revision 1.11  2003/12/12 01:51:39  korpela
 * Now using the opaque field in workunit to store SAH wuid.
 *
 * Revision 1.10  2003/12/03 23:46:41  korpela
 * WU count is now only for SAH workunits.
 *
 * Revision 1.9  2003/11/25 21:59:53  korpela
 * *** empty log message ***
 *
 * Revision 1.8  2003/11/11 06:20:30  korpela
 * Increased max fpops_max to prevent timeout on windows clients
 *
 * Revision 1.7  2003/10/25 18:19:44  korpela
 * *** empty log message ***
 *
 * Revision 1.6  2003/10/24 16:57:03  korpela
 * *** empty log message ***
 *
 * Revision 1.5  2003/09/26 20:48:52  jeffc
 * jeffc - merge in branch setiathome-4_all_platforms_beta.
 *
 * Revision 1.3.2.2  2003/09/22 19:00:31  korpela
 * *** empty log message ***
 *
 * Revision 1.3.2.1  2003/09/22 17:39:35  korpela
 * *** empty log message ***
 *
 * Revision 1.4  2003/09/22 17:05:38  korpela
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:44  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:36:00  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.3  2003/06/05 15:52:47  korpela
 *
 * Fixed coordinate bug that was using the wrong zenith angle from the telescope
 * strings when handling units from the gregorian.
 *
 * Revision 1.2  2003/06/03 01:01:18  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:23:44  korpela
 *
 * Again
 *
 * Revision 3.8  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.7  2003/04/10 17:32:25  korpela
 * *** empty log message ***
 *
 * Revision 3.6  2002/06/21 01:42:15  eheien
 * *** empty log message ***
 *
 * Revision 3.5  2001/11/07 00:51:47  korpela
 * Added splitter version to database.
 * Added max_wus_ondisk option.
 *
 * Revision 3.4  2001/08/17 22:20:54  korpela
 * *** empty log message ***
 *
 * Revision 3.3  2001/08/17 01:22:31  korpela
 * *** empty log message ***
 *
 * Revision 3.2  2001/08/17 01:16:53  korpela
 * *** empty log message ***
 *
 * Revision 3.1  2001/08/16 23:42:08  korpela
 * Mods for splitter to make binary workunits.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.18  2000/12/01 01:13:29  korpela
 * *** empty log message ***
 *
 * Revision 2.17  1999/06/07 21:00:52  korpela
 * *** empty log message ***
 *
 * Revision 2.16  1999/03/27  16:19:35  korpela
 * *** empty log message ***
 *
 * Revision 2.15  1999/03/05  01:47:18  korpela
 * Added data_class field.
 *
 * Revision 2.14  1999/02/22  22:21:09  korpela
 * added -nodb option
 *
 * Revision 2.13  1999/02/11  16:46:28  korpela
 * Added db access functions.
 *
 * Revision 2.12  1999/01/04  22:27:55  korpela
 * Updated return codes.
 *
 * Revision 2.11  1998/12/14  23:41:44  korpela
 * Added subband_base to work unit header.
 * Changed frequency calculation.
 *
 * Revision 2.10  1998/12/14  21:55:07  korpela
 * Added fft_len and ifft_len to work unit header.
 *
 * Revision 2.9  1998/11/13  23:58:52  korpela
 * Modified for move of name field between structures.
 *
 * Revision 2.8  1998/11/13  22:18:12  davea
 * *** empty log message ***
 *
 * Revision 2.7  1998/11/10 01:55:26  korpela
 * Server requires a CR at the end of a WU file
 * ???
 *
 * Revision 2.6  1998/11/10  00:02:44  korpela
 * Moved remaining wuheader fields into a WU_INFO structure.
 *
 * Revision 2.5  1998/11/05  21:33:02  korpela
 * Fixed angle_range bug.
 *
 * Revision 2.4  1998/11/05  21:18:41  korpela
 * Moved name field from header to seti header.
 *
 * Revision 2.3  1998/11/02  21:20:58  korpela
 * Modified for (internal) integer receiver ID.
 *
 * Revision 2.2  1998/11/02  18:45:39  korpela
 * Changed location of timecvt.h
 *
 * Revision 2.1  1998/11/02  16:38:29  korpela
 * Variable type changes.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.1  1998/10/27  01:01:16  korpela
 * Initial revision
 *
 *
 */
