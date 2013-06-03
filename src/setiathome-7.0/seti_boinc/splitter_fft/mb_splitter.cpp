/*
 *
 *  The splitter main program.  
 *
 * $Id: mb_splitter.cpp,v 1.1.2.6 2007/08/16 23:03:19 jeffc Exp $
 *
 */

#include "sah_config.h"
#undef USE_MYSQL
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <errno.h>

#include "boinc_db.h"
#include "backend_lib.h"
#include "sched_config.h"
#include "setilib.h"
#include "str_util.h"
#include "str_replace.h"
#include "splitparms.h"
#include "splittypes.h"
#include "mb_splitter.h"
#include "mb_validrun.h"
#include "mb_wufiles.h"
#include "mb_dotransform.h"
#include "message.h"
#include "sqlrow.h"
#include "sqlapi.h"
#include "db/db_table.h"
#include "db/schema_master.h"
#include "db/app_config.h"

extern "C" {
  int sqldetach();
}

char trigger_file_path[1024]="/disks/setifiler1/wutape/tapedir/splitter_stop";

SCHED_CONFIG boinc_config;
DB_APP app;
R_RSA_PRIVATE_KEY key;

// configuration tables
receiver_config rcvr;
settings splitter_settings;

// TEMPLATE DEFS ------------------------------------------------------
// IMPORTANT: a change to a template should *always* include a change
// to the template filename.  Only the result template is used now.
const char *wu_template_filename_id = "wu_0.xml";
const char *wu_template=
  "<file_info>\n"
  "  <number>0</number>\n"
  "</file_info>\n"
  "<workunit>\n"
  "  <file_ref>\n"
  "    <file_number>0</file_number>\n"
  "    <open_name>work_unit.sah</open_name>\n"
  "  </file_ref>\n"
  "</workunit>\n";

const char *result_template_filename_id = "result_0.xml";
const char *result_template=
  "<file_info>\n"
  "  <name><OUTFILE_0/></name>\n"
  "  <generated_locally/>\n"
  "  <upload_when_present/>\n"
  "  <max_nbytes>65536</max_nbytes>\n"
  "  <url>http://setiboincdata.ssl.berkeley.edu/sah_cgi/file_upload_handler</url>\n"
  "</file_info>\n"
  "<result>\n"
  "  <file_ref>\n"
  "    <file_name><OUTFILE_0/></file_name>\n"
  "    <open_name>result.sah</open_name>\n"
  "  </file_ref>\n"
  "</result>\n";
// END TEMPLATE DEFS --------------------------------------------------

std::vector<dr2_compact_block_t> tapebuffer;
std::map<seti_time,coordinate_t> coord_history;
std::vector<workunit> wuheaders;
char appname[256];
int max_wus_ondisk=MAX_WUS_ONDISK;
int nodb;
int noencode;
int resumetape;
int norewind;
int startblock;
int dataclass;
int atnight;
int gregorian;
int output_xml;
int polyphase;
int iters=-1;
int beam;
int pol;
int alfa;
int useanalysiscfgid = 0;
int usereceivercfgid = 0;
int userecordercfgid = 0;
int usesplittercfgid = 0;
int filter_window = 0;
double start_time;
double stop_time;
unsigned long minvfsbuf=-1;
char wd[1024];
//char * scidb = NULL;
char * projectdir = NULL;

APP_CONFIG sah_config;

//const char *result_template_filename="projectdir"/"SAH_APP_NAME"_result.tpl";
char result_template_filename[1024];
char result_template_filepath[1024];
char wu_template_filename[1024];

int check_for_halt();
int wait_until_night();
int check_free_disk_space();
int wait_for_db_wus_ondisk();

void cprint(char *p) {
  printf("%s\n",p);
}

/*  wulog: File for logging names of completed wu files */
/*  errorlog: File for logging errors                   */

FILE *wulog,*errorlog;
buffer_pos_t start_of_wu,end_of_wu;  /* position of start and end of wu in
                                      tape buffer */
int seqno;

void cleanup(void) {
  FILE *tmpfile;
  if ((tmpfile=fopen("seqno.dat","w"))) {
    fprintf(tmpfile,"%d\n",seqno);
    fclose(tmpfile);
  } else {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Unable to open seqno.dat for write\n");
  }
}


int process_command_line(int argc, char *argv[],char **tape_device,
                         int *norewind, int *startblock, int *resumetape,
                         int *nodb, int *dataclass, int *atnight,
                         int *max_wus_ondisk, char **projectdir,
			 int *iters, int *beam, int *pol, int *alfa,
                         int *useanalysiscfgid, int *usereceivercfgid,
                         int *userecordercfgid, int *usesplittercfgid) {
  int nargs=0,i;
  char *ep;
  strcpy(appname,SAH_APP_NAME);
  for (i=1;i<argc;i++) {
    if (argv[i][0]=='-') {
        if (!strncmp(argv[i],"-hanning",MAX(strlen(argv[i]),3))) {
          filter_window=2;
        } else
          if (!strncmp(argv[i],"-welch",MAX(strlen(argv[i]),3))) {
            filter_window=1;
          } else
            if (!strncmp(argv[i],"-xml",MAX(strlen(argv[i]),3))) {
              output_xml=1;
            } else
              if (!strncmp(argv[i],"-atnight",MAX(strlen(argv[i]),7))) {
                *atnight=1;
              } else
                if (!strncmp(argv[i],"-nodb",MAX(strlen(argv[i]),4))) {
                  if (*resumetape) {
                    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"-nodb and -resumetape are exclusive\n");
                    return(1);
                  }
                  *nodb=1;
                } else
                  if (!strncmp(argv[i],"-gregorian",MAX(strlen(argv[i]),2))) {
                    gregorian=1;
                  } else
                    if (!strncmp(argv[i],"-resumetape",MAX(strlen(argv[i]),2))) {
                      if (*startblock || *norewind) {
		        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"-startblock and -norewind can\'t be used with -resumetape\n");
                        return(1);
                      }
                      if (*nodb) {
                        log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"-nodb and -resumetape are exclusive\n");
                        return(1);
                      }
                      *resumetape=1;
                    } else
                      if (!strncmp(argv[i],"-norewind",MAX(strlen(argv[i]),4))) {
                        if (*startblock || *resumetape) {
                          return(1);
                        }
                        *norewind=1;
                      } else
                        if ((ep=strchr(argv[i],'='))) {
                          if (!strncmp(argv[i],"-appname",MAX(ep-argv[i],3))) {
                            strlcpy(appname,ep+1,sizeof(appname));
                          } else if (!strncmp(argv[i],"-trigger_file_path",MAX(ep-argv[i],2))) {
			    char *fe=ep+1;
			    memset(trigger_file_path,0,sizeof(trigger_file_path));
			    while (isgraph(*(fe++))) trigger_file_path[fe-ep-2]=*fe ;
                          } else if (!strncmp(argv[i],"-alfa",MAX(ep-argv[i],3))) {
			    sscanf(ep+1,"%d,%d",beam,pol);
			    if (((*beam<0) || (*beam>6)) ||
			        ((*pol<0) || (*pol>1))) {
                                 log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"ALFA receivers must be specified with beam (0-6) and polarization (0-1), i.e. -alfa=4,0\n");
				 exit(EXIT_FAILURE);
		            }

			    *alfa=1;
                          } else if (!strncmp(argv[i],"-iterations",MAX(ep-argv[i],2))) {
			    sscanf(ep+1,"%d",iters);
			  } else if (!strncmp(argv[i],"-startblock",MAX(ep-argv[i],2))) {
                            if (*norewind || *resumetape) {
                              return(1);
                            }
                            sscanf(ep+1,"%d",startblock);
                          } else if (!strncmp(argv[i],"-max_wus_ondisk",MAX(ep-argv[i],2))) {
                              sscanf(ep+1,"%d",max_wus_ondisk);
                          } else if (!strncmp(argv[i],"-dataclass",MAX(ep-argv[i],2))) {
                                sscanf(ep+1,"%d",dataclass);
                          } else if (!strncmp(argv[i],"-projectdir",MAX(ep-argv[i],2))) {
			          *projectdir=(char *)calloc(strlen(ep+1)+5,1);
				  strlcpy(*projectdir,ep+1,strlen(ep+1)+5);
                                  strlcat(*projectdir,"/",strlen(ep+1)+5);
                          } else if (!strncmp(argv[i],"-analysis_config",MAX(ep-argv[i],2))) {
                              sscanf(ep+1,"%d",useanalysiscfgid);
                          } else if (!strncmp(argv[i],"-recorder_config",MAX(ep-argv[i],2))) {
                              sscanf(ep+1,"%d",userecordercfgid);
                          } else if (!strncmp(argv[i],"-receiver_config",MAX(ep-argv[i],2))) {
                              sscanf(ep+1,"%d",usereceivercfgid);
                          } else if (!strncmp(argv[i],"-splitter_config",MAX(ep-argv[i],2))) {
                              sscanf(ep+1,"%d",usesplittercfgid);
                          } else {
                            return(1);
                          }
                        } else {
                          return(1);
                        }
    } else {
      *tape_device=argv[i];
      nargs++;
    }
  }
  //return(nargs!=1);

  // check for required cmd line 
  //if (! *scidb) return (1);
  if (! *projectdir) return (1);
  if (! *tape_device) return (1);

  return 0;
}

int main(int argc, char *argv[]) {
  int tape_fd;
  char *tape_device;
  FILE *tmpfile;
  int retval;
  char keyfile[1024];
  char tmpstr[1024];
  char buf[1024];


  /* Process command line arguments */
  if (process_command_line(argc,argv,&tape_device,&norewind,&startblock,&resumetape,
                           &nodb,&dataclass,&atnight,&max_wus_ondisk,&projectdir,&iters,
			   &beam,&pol,&alfa,&useanalysiscfgid,&usereceivercfgid,
                           &userecordercfgid,&usesplittercfgid))  {
    fprintf(stderr,"Usage: splitter tape_device -projectdir=s [-atnight] [-nodb]\n"
    "[-xml] [-gregorian] [-resumetape | -norewind | -startblock=n] [-dataclass=n]\n"
    "[-max_wus_on_disk=n] [-iterations=n] [-trigger_file_path=filename]\n"
    "[-alfa=beam,pol] [-analysis_config=id] [-receiver_config=id]\n"
    "[-recorder_config=id] [-splitter_config=id]\n");
    exit(EXIT_FAILURE);
  }

  // MATTL
  if (blanking_bit == SOFTWARE_BLANKING_BIT) log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"blanking bit: %d (SOFTWARE)\n",blanking_bit);
  else log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"blanking bit: %d (HARDWARE)\n",blanking_bit);

  /* Open log files */
  log_messages.set_debug_level(3);

  retval = sah_config.parse_file(projectdir);
  if (retval) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Can't parse config file\n");
    exit(EXIT_FAILURE);
  } else {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using configuration:\n");
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"scidb_name          = %s\n", sah_config.scidb_name);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"max_wus_ondisk      = %d\n", sah_config.max_wus_ondisk);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"min_quorum          = %d\n", sah_config.min_quorum);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"target_nresults     = %d\n", sah_config.target_nresults);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"max_error_results   = %d\n", sah_config.max_error_results);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"max_success_results = %d\n", sah_config.max_success_results);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"max_total_results   = %d\n", sah_config.max_total_results);
  }

  // Will initially open 
  if (!db_change(sah_config.scidb_name)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Could not open science database %s\n", sah_config.scidb_name);
    if (!nodb) exit(EXIT_FAILURE);
  } else {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using science database %s\n", sah_config.scidb_name);
  }


  boinc_config.parse_file(projectdir);
  log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using boinc config dir  %s\n", projectdir);
  log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using boinc database %s\n", boinc_config.db_name);

  // check / commit result template
  strcpy(result_template_filename, "templates/");
  strcat(result_template_filename, appname);
  strcat(result_template_filename, "_");
  strcat(result_template_filename, result_template_filename_id);
  strcpy(result_template_filepath, projectdir);
  strcat(result_template_filepath, result_template_filename);
  log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"%s %s\n", result_template_filename, result_template_filepath);
  if (!(tmpfile=fopen(result_template_filepath,"r"))) {
    if (!(tmpfile=fopen(result_template_filepath,"w"))) {
      log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Cannot open result file template : %s\n", result_template_filepath);
      exit(1);
    }
    fprintf(tmpfile,result_template);
  }
  fclose(tmpfile);

  if (boinc_db.open(boinc_config.db_name,boinc_config.db_host,boinc_config.db_user,boinc_config.db_passwd)) {
    boinc_db.print_error("boinc_db.open");
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"boinc_db.open\n");
    exit(1);
  }

  char tmpquery[128];
  sprintf(tmpquery,"where name ='%s'",appname);
  if (app.lookup(tmpquery)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Error looking up appname\n");
    boinc_db.print_error("boinc_app lookup");
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"boinc_app lookup\n");
    exit(1);
  } else {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Splitting for application %s (ID = %d)\n", appname, app.id);
  }

  boinc_db.close();

  telescope_id tel=
           channel_to_receiverid[beam_pol_to_channel[bmpol_t(beam,pol)]];

  sprintf(buf,"where active=%d and receiver_cfg=(select id from receiver_config where s4_id=%d)",app.id,AO_ALFA_0_0);
  if (usereceivercfgid > 0) { 
    sprintf(buf,"where active=%d and receiver_cfg=%d",app.id,usereceivercfgid);
    }
  if (!splitter_settings.fetch(std::string(buf))) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to find active settings for app.id=%d\n",app.id);
    exit(1);
  }

  sprintf(buf,"where s4_id=%d and id>=%d",tel,splitter_settings.receiver_cfg.id);
  if (usereceivercfgid > 0) { sprintf(buf,"where id=%d",usereceivercfgid); }
  rcvr.fetch(buf);
  if (usereceivercfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using receiver cfg id: %d (set by user)\n",usereceivercfgid);
    splitter_settings.receiver_cfg = usereceivercfgid; 
    splitter_settings.receiver_cfg->fetch();
  } else { 
    splitter_settings.receiver_cfg->fetch(buf);
  }
  if (userecordercfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using recorder cfg id: %d (set by user)\n",userecordercfgid);
    splitter_settings.recorder_cfg = userecordercfgid; 
  }
  splitter_settings.recorder_cfg->fetch(); 
  if (usesplittercfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using splitter cfg id: %d (set by user)\n",usesplittercfgid);
    splitter_settings.splitter_cfg = usesplittercfgid; 
  }
  splitter_settings.splitter_cfg->fetch();
  if (useanalysiscfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using analysis cfg id: %d (set by user)\n",useanalysiscfgid);
    splitter_settings.analysis_cfg = useanalysiscfgid;
  } 
  splitter_settings.analysis_cfg->fetch();

  log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL, "settings: \n\n%s\n", splitter_settings.print_xml(1,1,0).c_str());
 
  strcpy(keyfile, projectdir);
  strcat(keyfile, "/keys/upload_private");
  if (read_key_file(keyfile,key)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Error reading keyfile.\n");
    exit(1);
  }

  check_for_halt();

  if ((tape_fd=open(tape_device,O_RDONLY|0x2000, 0777))<0) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to open tape device\n");
    exit(EXIT_FAILURE);
  }

  // In early tapes there was a bug where the first N blocks would be duplicates
  // of data from previous files.  So we need to fast forward until we see a
  // frame sequence number of 1.
  int i,readbytes=HeaderSize;
  dataheader_t header;
  header.frameseq=100000;

  while ((readbytes==HeaderSize) && (header.frameseq>10)) {
    char buffer[HeaderSize];
    int nread;
    readbytes=0;
    while ((readbytes!=HeaderSize) && (nread = read(tape_fd,buffer,HeaderSize-readbytes))) { 
	    readbytes+=nread;
    }
    if (nread < 0) {
	log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"File error %d.\n", errno);
	exit(1);
    }
    if (readbytes == HeaderSize) {
      header.populate_from_data(buffer);
      if (header.frameseq>10) {
        lseek64(tape_fd,DataSize,SEEK_CUR);
      } else {
        lseek64(tape_fd,-1*(off64_t)HeaderSize,SEEK_CUR);
      }
    }
  }

  if (readbytes != HeaderSize) {
    // we fast forwarded through the entire tape without finding the first frame
    // maybe this is one of the really early tapes that was split into chunks.
    lseek64(tape_fd,0,SEEK_SET);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Warning: First block not found\n");
  }

  if (resumetape) {
    tape thistape;
    thistape.id=0; 
    readbytes=HeaderSize;
    sprintf(buf,"%d",rcvr.s4_id-AO_ALFA_0_0);
    if (thistape.fetch(std::string("where name=\'")+header.name+"\' and beam="+buf)) {
      log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"Resuming tape %s beam %d pol %d\n",thistape.name,beam,pol );
      while ((readbytes==HeaderSize) && (header.dataseq!=thistape.last_block_done)) {
        int nread=0;
        char buffer[HeaderSize];
        readbytes=0;
        while ((readbytes!=HeaderSize) &&
	      ((nread = read(tape_fd,buffer,HeaderSize-readbytes)) > 0 )) {
	    readbytes+=nread;
        }
        if (readbytes == HeaderSize) {
          header.populate_from_data(buffer);
          if (header.dataseq!=thistape.last_block_done) {
            lseek64(tape_fd,(off64_t)(DataSize+HeaderSize)*(thistape.last_block_done-header.dataseq)-HeaderSize,SEEK_CUR);
          } else {
            lseek64(tape_fd,-1*(off_t)HeaderSize,SEEK_CUR);
            log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Found starting point");
          }
	}
	if (nread == 0) {
	  log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"End of file.\n");
	  exit(0);
        }
        if (nread < 0) {
	  log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"File error %d.\n",errno);
	  exit(1);
	}
      }
    }
  }

  /* Start of main loop */


  atexit(cleanup);

  getcwd(wd,1024);

  if (polyphase) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Polyphase not implemented\n" );
    exit(1);
#if 0
    int filter_len;

    filter_len = P_FFT_LEN*N_WINDOWS;

    filter_r = (double *)malloc(sizeof(double)*filter_len);
    filter_i = (double *)malloc(sizeof(double)*filter_len);

    f_data = (float *)malloc(sizeof(float)*P_FFT_LEN*2);

    make_FIR(filter_len, N_WINDOWS, filter_window, filter_r);
    make_FIR(filter_len, N_WINDOWS, filter_window, filter_i);

    /*
    int i;
    float *data;
    data = (float*)malloc(sizeof(float)*2*2048);
    for (double n=-20;n<20;n+=.05) {
    for (i=0;i<2*2048;i+=2) {
        data[i] = cos(2*(24+n)*3.1415926535*i/(2*2048));
        data[i+1] = sin(2*(24+n)*3.1415926535*i/(2*2048));
    }
    polyphase_seg(data);
    //fprintf( stderr, "%f\n", 3+n/8);
    }
    exit(0);
    for(i=0;i<filter_len;i++)
      printf( "%f\n", filter_r[i] );
    exit(0);*/
#endif
  }
  log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"Entering loop\n" );

  blanking_filter<complex<signed char> > blanker_filter;
  if (strcmp(splitter_settings.splitter_cfg->blanker_filter, "randomize") == 0) {
    log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"Setting blanker filter to randomize\n" );
    blanker_filter = randomize;
  } else {
    log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"Setting blanker filter to null\n" );
    blanker_filter = NULLC;      // no blanking
  }
  while (iters-- && 
    ((512-tapebuffer.size())==seti_StructureDr2Data(tape_fd, beam, pol,
            512-tapebuffer.size(), tapebuffer, blanker_filter))) {
    /* check if we should be running now */
    fflush(stderr);
    if (atnight) wait_until_night();

  sprintf(buf,"where active=%d and receiver_cfg=(select id from receiver_config where s4_id=%d)",app.id,AO_ALFA_0_0);
  if (usereceivercfgid > 0) { 
    sprintf(buf,"where active=%d and receiver_cfg=%d",app.id,usereceivercfgid);
    }
  if (!splitter_settings.fetch(std::string(buf))) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to find active settings for app.id=%d\n",app.id);
    exit(1);
  }

  sprintf(buf,"where s4_id=%d and id>=%d",tel,splitter_settings.receiver_cfg.id);
  if (usereceivercfgid > 0) { sprintf(buf,"where id=%d",usereceivercfgid); }
  rcvr.fetch(buf);
  if (usereceivercfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using receiver cfg id: %d (set by user)\n",usereceivercfgid);
    splitter_settings.receiver_cfg = usereceivercfgid; 
    splitter_settings.receiver_cfg->fetch();
  } else { 
    splitter_settings.receiver_cfg->fetch(buf);
  }
  if (userecordercfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using recorder cfg id: %d (set by user)\n",userecordercfgid);
    splitter_settings.recorder_cfg = userecordercfgid; 
  }
  splitter_settings.recorder_cfg->fetch(); 
  if (usesplittercfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using splitter cfg id: %d (set by user)\n",usesplittercfgid);
    splitter_settings.splitter_cfg = usesplittercfgid; 
  }
  splitter_settings.splitter_cfg->fetch();
  if (useanalysiscfgid > 0) { 
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Using analysis cfg id: %d (set by user)\n",useanalysiscfgid);
    splitter_settings.analysis_cfg = useanalysiscfgid;
  } 
  splitter_settings.analysis_cfg->fetch();

  log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG, "settings: \n\n%s\n", splitter_settings.print_xml(1,1,0).c_str());

    check_for_halt();

    // Make sure we have enough free disk space
    check_free_disk_space();

    // Wait for ondisk wus in the database to drop below the threshold
    if (!nodb) wait_for_db_wus_ondisk();
    //fprintf( stderr, "Read data\n" );
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"less than max wus on disk, continuing\n" );
    fflush(stderr);

    //printf( "Read tape data, analyzing\n" );
    check_for_halt();
    
    //check for an uninterrupted run
    if (valid_run(tapebuffer,splitter_settings.receiver_cfg->min_vgc)) {
      std::vector<dr2_compact_block_t>::iterator i=tapebuffer.begin();
      // insert telescope coordinates into the coordinate history.
      // this should be converted to a more accurate routine.
      for (;i!=tapebuffer.end();i++) {
        coord_history[i->header.coord_time].ra   = i->header.ra;
        coord_history[i->header.coord_time].dec  = i->header.dec;
        coord_history[i->header.coord_time].time = i->header.coord_time.jd().uval();
      }
      if (make_wu_headers(tapebuffer,tel,wuheaders)) {
        int child_pid=-1;
        switch (polyphase) {
          case 1:
	    #if 0
            do_polyphase(&start_of_wu,&end_of_wu);
	    #endif
            break;
          default:
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"doing transform..." );
            do_transform(tapebuffer);
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG," done\n" );
        }
        wait(0);
        if (!nodb) sql_finish();

        do {
          sleep(1);
          child_pid=fork();
	  if (child_pid<0) log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"splitter cannot fork");
          //fprintf( stderr, "child pid: %d\n", child_pid);
        } while (child_pid<0);

        if (!child_pid) {
          if (!nodb) {
            sqldetach();
            while (!sql_database(sah_config.scidb_name)) {
		//fprintf(stderr,"child sleeping\n");
		sleep(10);
	    }
          }
          rename_wu_files();
          if (!nodb) sql_finish();
          _exit(0);
        }

        if (!nodb) {
          while (!sql_database(sah_config.scidb_name)) {
		//fprintf(stderr,"parent sleeping\n");
		sleep(10);
	      }
        }
      }
    }
  }

  // iters is initalized to -1.  If the -iters flag is specified on the command
  // line, iters is re-initialized to the user desired number of interations.
  // Each time a WUG is proccessed iters is decremineted.  If the user specifies
  // iters and we do that number of iterations without reaching EOF then iters
  // here will be zero.  !Zero means we have reached EOF prior to performing the user 
  // desired number of iterations.  
  //
  // On the other hand, if iters is not specified by the user, iters is decrmented 
  // downward from -1. It will never be zero in this case.  But since we are not 
  // limiting the number of iterations, the only way will get here is if we reach EOF.  
  //
  // Thus in both cases, a non-zero iters means we have reached EOF.
  if (iters != 0) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"End of file.\n");
    // clean stop at EOF
    return (EXIT_NORMAL_EOF);
  } else {
    // clean stop, but not EOF (iters satisfied or triggered stop)
    // (A return of 1 is an error exit somewhere else in the code.)
    return(EXIT_NORMAL_NOT_EOF); 
  }
}

int check_for_halt() {
  FILE *tf;

  tf = fopen(trigger_file_path, "r");
  // tf=0;
  if (tf != NULL) {  // Stop program
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Found splitter_stop, exiting.\n" );
    exit(EXIT_NORMAL_NOT_EOF);
  }
  return(0); // Keep going
}

int wait_until_night() {
  time_t t;
  struct tm *lt;
  do {
    t=time(0);
    lt=localtime(&t);
    // Don't run M-F 8AM-6PM
    if ((lt->tm_wday>0) && (lt->tm_wday<6) && (lt->tm_hour>8) && (lt->tm_hour<18)) {
      sleep(100);
    }
  } while ((lt->tm_wday>0) && (lt->tm_wday<6) && (lt->tm_hour>8) && (lt->tm_hour<18));
  return 0;
}

int check_free_disk_space() {
  struct statvfs vfsbuf;

  /* check disk free space in working directory */
  statvfs("wu_inbox/.",&vfsbuf);
  if (vfsbuf.f_frsize==0) vfsbuf.f_frsize=512;
  if (vfsbuf.f_bavail < (100000*1024/vfsbuf.f_frsize)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Not enough free disk space in working directory to continue\n");
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Not enough free disk space in working directory to continue\n");
    exit(EXIT_FAILURE);
  }
  
  /* check free disk space in download directory tree */
  std::string download_dir(boinc_config.download_dir);
  int waited=0;
  download_dir+="/.";
  /* wait here if the disk is more than N% full */ 
  statvfs(download_dir.c_str(),&vfsbuf);
  if (vfsbuf.f_blocks > vfsbuf.f_bfree*100/sah_config.min_disk_free_pct) {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Waiting for free space in download directory\n");
    while (vfsbuf.f_blocks > vfsbuf.f_bfree*100/sah_config.min_disk_free_pct) {
      waited+=10;
      sleep(10);
      statvfs(download_dir.c_str(),&vfsbuf);
    }
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Waited %d seconds\n", waited);
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Continuing...\n");
  }
  return 0;
}

int wait_for_db_wus_ondisk() {
  //int wus_ondisk,rv;
  DB_STATE_COUNTS state_counts;
  int retval;

  // No need to check the DB for every single WUG iteration.
  static int check_count = 100;
  if (check_count < 100) {
	check_count++;
	return 0;
  } else {
	check_count = 0;
  } 
	
  if (boinc_db.open(boinc_config.db_name,boinc_config.db_host,boinc_config.db_user,boinc_config.db_passwd)) {
    boinc_db.print_error("boinc_db.open");
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"boinc_db.open\n");
    exit(1);
  }
#if 0
  retval = boinc_db.set_isolation_level(READ_UNCOMMITTED);
  if (retval) {
    log_messages.printf(MSG_CRITICAL, "boinc_db.set_isolation_level: %d; %s\n", rv, boinc_db.error_string());
    exit(EXIT_FAILURE);
  }
  else {
    log_messages.printf(MSG_NORMAL, "setting read uncommitted isolation level\n");
  }
#endif
  do {
    char query[1024];
    sprintf(query,"where appid=%d",app.id);
    retval=state_counts.lookup(query);
    if (retval) {
      boinc_db.print_error("state_counts.lookup");
      log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"DB Error, unable to count workunits on disk\n");
      exit(EXIT_FAILURE);
    }
    check_for_halt();
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"%d WUs ondisk\n", state_counts.result_server_state_2);
    if (state_counts.result_server_state_2>sah_config.max_wus_ondisk) sleep(600);

#if 0
    sprintf(query,"where appid=%d and server_state=2",app.id);
    rv=boinc_result.count(wus_ondisk,query);
    if (rv) {
      boinc_db.print_error("boinc_result.count");
      log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"DB Error, unable to count workunits on disk\n");
      exit(EXIT_FAILURE);
    }
    check_for_halt();
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"%d WUs ondisk\n", wus_ondisk);
    if (wus_ondisk>sah_config.max_wus_ondisk) sleep(600);
  } while (wus_ondisk>sah_config.max_wus_ondisk);
#endif

  } while (state_counts.result_server_state_2>sah_config.max_wus_ondisk);

  boinc_db.close();
  return 0;
}

/*
 * $Log: mb_splitter.cpp,v $
 * Revision 1.1.2.6  2007/08/16 23:03:19  jeffc
 * *** empty log message ***
 *
 * Revision 1.1.2.5  2007/08/10 19:29:40  jeffc
 * *** empty log message ***
 *
 * Revision 1.1.2.4  2007/06/07 20:01:52  mattl
 * *** empty log message ***
 *
 * Revision 1.1.2.3  2007/06/06 15:58:29  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.2  2007/04/25 17:27:30  korpela
 * *** empty log message ***
 *
 * Revision 1.1.2.1  2006/12/14 22:24:43  korpela
 * *** empty log message ***
 *
 * Revision 1.22.2.4  2006/11/08 20:16:26  vonkorff
 * Fixes an error message.
 *
 * Revision 1.22.2.3  2006/11/07 19:26:37  vonkorff
 * Fixed lcgf
 *
 * Revision 1.22.2.2  2006/01/13 00:37:58  korpela
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
 * Revision 1.22.2.1  2005/08/01 21:16:25  jeffc
 * *** empty log message ***
 *
 * Revision 1.22  2005/01/27 23:03:27  mattl
 *
 * commented out tf=0 in check_for_halt
 *
 * Revision 1.21  2004/12/27 20:48:54  jeffc
 * *** empty log message ***
 *
 * Revision 1.20  2004/08/12 15:45:41  jeffc
 * *** empty log message ***
 *
 * Revision 1.19  2004/07/09 22:35:39  jeffc
 * *** empty log message ***
 *
 * Revision 1.18  2004/06/27 21:07:04  jeffc
 * *** empty log message ***
 *
 * Revision 1.17  2004/06/20 18:56:48  jeffc
 * *** empty log message ***
 *
 * Revision 1.16  2004/06/18 23:23:32  jeffc
 * *** empty log message ***
 *
 * Revision 1.15  2004/06/16 20:57:19  jeffc
 * *** empty log message ***
 *
 * Revision 1.14  2004/04/08 22:25:47  jeffc
 * *** empty log message ***
 *
 * Revision 1.13  2004/01/22 00:57:53  korpela
 * *** empty log message ***
 *
 * Revision 1.12  2004/01/01 18:42:01  korpela
 * *** empty log message ***
 *
 * Revision 1.11  2003/12/03 23:46:41  korpela
 * WU count is now only for SAH workunits.
 *
 * Revision 1.10  2003/11/25 21:59:52  korpela
 * *** empty log message ***
 *
 * Revision 1.9  2003/11/01 20:54:02  korpela
 * *** empty log message ***
 *
 * Revision 1.8  2003/10/24 16:57:03  korpela
 * *** empty log message ***
 *
 * Revision 1.7  2003/09/26 20:48:51  jeffc
 * jeffc - merge in branch setiathome-4_all_platforms_beta.
 *
 * Revision 1.5.2.2  2003/09/23 00:49:12  korpela
 * *** empty log message ***
 *
 * Revision 1.5.2.1  2003/09/22 17:39:34  korpela
 * *** empty log message ***
 *
 * Revision 1.6  2003/09/22 17:05:38  korpela
 * *** empty log message ***
 *
 * Revision 1.5  2003/09/13 20:48:38  korpela
 * directory reorg.  Moved client files to ./client
 *
 * Revision 1.4  2003/09/11 18:53:38  korpela
 * *** empty log message ***
 *
 * Revision 1.3  2003/08/13 23:18:47  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:42  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/07/29 20:35:50  korpela
 *
 * renames .C files to .cpp
 *
 * Revision 1.2  2003/06/03 01:01:17  korpela
 *
 * First working splitter under CVS.
 *
 * Revision 1.1  2003/06/03 00:23:40  korpela
 *
 * Again
 *
 * Revision 3.6  2003/05/21 00:41:42  korpela
 * *** empty log message ***
 *
 * Revision 3.5  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.4  2003/04/09 17:46:54  korpela
 * *** empty log message ***
 *
 * Revision 3.3  2002/06/20 22:09:17  eheien
 * *** empty log message ***
 *
 * Revision 3.2  2001/11/07 00:51:47  korpela
 * Added splitter version to database.
 * Added max_wus_ondisk option.
 *
 * Revision 3.1  2001/08/16 23:42:08  korpela
 * Mods for splitter to make binary workunits.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.7  2000/12/01 01:13:29  korpela
 * *** empty log message ***
 *
 * Revision 2.6  1999/06/07 21:00:52  korpela
 * *** empty log message ***
 *
 * Revision 2.5  1999/03/27  16:19:35  korpela
 * *** empty log message ***
 *
 * Revision 2.4  1999/03/05  01:47:18  korpela
 * Added dataclass paramter.
 *
 * Revision 2.3  1999/02/22  22:21:09  korpela
 * added -nodb option
 *
 * Revision 2.2  1999/02/11  16:46:28  korpela
 * Added some db access functions.
 *
 * Revision 2.1  1998/11/02  16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.2  1998/10/30  20:26:03  korpela
 * Bug Fixes.  Now mostly working.
 *
 * Revision 1.1  1998/10/27  00:58:16  korpela
 * Initial revision
 *
 *
 */
