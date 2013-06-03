/*
 *
 *  The splitter main program.  
 *
 * $Id: splitter.cpp,v 1.22.2.6 2007/06/06 15:58:30 korpela Exp $
 *
 */

#include "sah_config.h"
#undef USE_MYSQL
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/wait.h>

#include "boinc_db.h"
#include "crypt.h"
#include "backend_lib.h"
#include "sched_config.h"
#include "splitparms.h"
#include "splittypes.h"
#include "splitter.h"
#include "validrun.h"
#include "makebufs.h"
#include "readtape.h"
#include "readheader.h"
#include "wufiles.h"
#include "dotransform.h"
#include "polyphase.h"
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

unsigned char *tapebuffer;     /* A buffer for a tape section */
workunit wuheaders[NSTRIPS];
tapeheader_t tapeheaders[TAPE_FRAMES_IN_BUFFER];
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
int records_in_buffer;

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
			 int *iters) {
  int nargs=0,i;
  char *ep;
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
                          if (!strncmp(argv[i],"-trigger_file_path",MAX(ep-argv[i],2))) {
			    char *fe=ep+1;
			    memset(trigger_file_path,0,sizeof(trigger_file_path));
			    while (isgraph(*(fe++))) trigger_file_path[fe-ep-2]=*fe ;
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
			          *projectdir=calloc(strlen(ep+1)+5,1)
    				  strlcpy(*projectdir,ep+1,strlen(ep+1)+5);
				  strlcat(*projectdir,"/",strlen(ep+1)+5);
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
  char *tape_device;
  FILE *tmpfile;
  int retval;
  char keyfile[1024];
  char tmpstr[1024];


  /* Process command line arguments */
  if (process_command_line(argc,argv,&tape_device,&norewind,&startblock,&resumetape,
                           &nodb,&dataclass,&atnight,&max_wus_ondisk,&projectdir,&iters))  {
    fprintf(stderr,"Usage: splitter tape_device -projectdir=s [-atnight] [-nodb]\n"
    "[-xml] [-gregorian] [-resumetape | -norewind | -startblock=n] [-dataclass=n]\n"
    "[-max_wus_on_disk=n] [-iterations=n] [-trigger_file_path=filename]\n");
    exit(EXIT_FAILURE);
  }

  /* Open log files */
  log_messages.set_debug_level(3);
  log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"opening log files\n");
  if (!(wulog=fopen("wu.log","wa")) || !(errorlog=freopen("error.log","wa",stderr))) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to open log files!\n");
    exit(EXIT_FAILURE);
  }

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
  strcpy(result_template_filename, "templates/"SAH_APP_NAME"_");
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

  if (app.lookup("where name ='"SAH_APP_NAME"'")) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Error looking up appname\n");
    boinc_db.print_error("boinc_app lookup");
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"boinc_app lookup\n");
    exit(1);
  } else {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Splitting for application %s (ID = %d)\n", SAH_APP_NAME, app.id);
  }

  boinc_db.close();

  strcpy(keyfile, projectdir);
  strcat(keyfile, "/keys/upload_private");
  if (read_key_file(keyfile,key)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Error reading keyfile.\n");
    exit(1);
  }

  // if (!getenv("S4_RECEIVER_CONFIG")) putenv(s4cnf_env);
  check_for_halt();

  /* Process command line arguments */
  //if (process_command_line(argc,argv,&tape_device,&norewind,&startblock,&resumetape,
  //                         &nodb,&dataclass,&atnight,&max_wus_ondisk,&scidb,&projectdir))  {
  //  fprintf(stderr,"Usage: splitter tape_device [-atnight] [-nodb] [-xml] [-gregorian] [-resumetape | -norewind | -startblock=n] [-dataclass=n]\n");
  //  exit(EXIT_FAILURE);
  //}


  /* Connect to database */
  //if (!sql_database(SCIENCE_DB)) {
  //  fprintf(stderr,"Unable to connect to database.\n");
  //  if (!nodb) exit(EXIT_FAILURE);
  //}





  /*
   * Allocate temp files for tape buffer.  
   */
  makebuffers(&tapebuffer);

  if (!open_tape_device(tape_device)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Unable to open tape device\n");
    exit(EXIT_FAILURE);
  }

  /*
   * Get sequence number 
   */

  if ((tmpfile=fopen("seqno.dat","r"))) {
    fscanf(tmpfile,"%d",&seqno);
    fclose(tmpfile);
  } else {
    seqno=0;
  }
  /* Start of main loop */

  records_in_buffer=0;
  start_of_wu.frame=0;
  start_of_wu.byte=0;
  end_of_wu.frame=0;
  end_of_wu.byte=0;

  atexit(cleanup);

  getcwd(wd,1024);

  if (polyphase) {
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
  }

  log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"Entering loop\n" );
  while (iters-- && 
         fill_tape_buffer(tapebuffer+records_in_buffer*TAPE_RECORD_SIZE,
           TAPE_RECORDS_IN_BUFFER-records_in_buffer)) {
    /* check if we should be running now */
    fflush(stderr);
    if (atnight) wait_until_night();

    check_for_halt();

    // Make sure we have enough free disk space
    check_free_disk_space();

    // Wait for ondisk wus in the database to drop below the threshold
    if (!nodb) wait_for_db_wus_ondisk();
    //fprintf( stderr, "Read data\n" );
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"less than max wus on disk, continuing\n" );
    fflush(stderr);

    //printf( "Read tape data, analyzing\n" );
    parse_tape_headers(tapebuffer, &(tapeheaders[0]));
    check_for_halt();
    if (valid_run(tapeheaders,&start_of_wu,&end_of_wu)) {
      if (make_wu_headers(tapeheaders,wuheaders,&start_of_wu)) {
        int child_pid=-1;
        switch (polyphase) {
          case 1:
            do_polyphase(&start_of_wu,&end_of_wu);
            break;
          default:
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG,"doing transform..." );
            do_transform(&start_of_wu,&end_of_wu);
            log_messages.printf(SCHED_MSG_LOG::MSG_DEBUG," done\n" );
        }
        wait(0);
        if (!nodb) sql_finish();
/*
        do {
          sleep(1);
          child_pid=fork();
	  if (child_pid<0) log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"splitter cannot fork");
          //fprintf( stderr, "child pid: %d\n", child_pid);
        } while (child_pid<0);
*/
        child_pid=0;
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
//          _exit(0);
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
return(0);

}

int check_for_halt() {
  FILE *tf;

  tf = fopen(trigger_file_path, "r");
  // tf=0;
  if (tf != NULL) {  // Stop program
    log_messages.printf(SCHED_MSG_LOG::MSG_CRITICAL,"Found splitter_stop, exiting.\n" );
    exit(1);
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

  /* check disk free space */
  statvfs("wu_inbox/.",&vfsbuf);
  if (vfsbuf.f_bavail < (100000*1024/vfsbuf.f_frsize)) {
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Not enough free disk space in working directory to continue\n");
    log_messages.printf(SCHED_MSG_LOG::MSG_NORMAL,"Not enough free disk space in working directory to continue\n");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int wait_for_db_wus_ondisk() {
  int wus_ondisk,rv;

  // The boinc db query below takes a long time.  Until we fix this,
  // we will do it only every 100 times into this routine.  All other
  // calls here will assume that we need to add WUs.  -- jeffc
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
  do {
    DB_RESULT boinc_result;
    char query[1024];
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

  return 0;
}

/*
 * $Log: splitter.cpp,v $
 * Revision 1.22.2.6  2007/06/06 15:58:30  korpela
 * *** empty log message ***
 *
 * Revision 1.22.2.5  2006/12/14 22:24:48  korpela
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
