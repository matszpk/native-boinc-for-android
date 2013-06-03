#! /bin/sh 
PATH=/opt/misc:/opt/misc/bin:/usr/ccs/bin:/usr/openwin/bin:/usr/openwin/bin/xview:/usr/lang:/usr/ucb:/bin:/usr/bin:/usr/local/bin:/usr/hosts:/usr/bin/X11:/usr/casual/sparc:/usr/etc:/usr/local/lib/tex/bin:/usr/local/lib/frame/bin:/disks/asimov/a/apps/informix/bin:.:/disks/albert/d/office/bin:/usr/ucb:/disks/albert/e/users/eurd/data_analysis/bin:/usr/local/lib/g++-include.bak:/disks/siren/a/users/seti/s4/siren/bin:/disks/albert/d/orfeus:/disks/albert/d/chips:/disks/asimov/a/apps/informix/bin:/usr/local/tex
export PATH
LD_LIBRARY_PATH=/lib:/disks/asimov/a/apps/informix/lib:/disks/asimov/a/apps/informix/lib/esql::/usr/ucblib:/lib:/usr/lib:/usr/openwin/lib:/usr/ccs/lib:/usr/local/gcc/lib:/disks/asimov/a/lang/gnu/H-sparc-sun-solaris2/lib:/opt/misc/lib:/usr/local/lib:/disks/ellie/a/users/korpela/lib:/usr/dt/lib
export LD_LIBRARY_PATH
INFORMIXDIR=/disks/asimov/a/apps/informix/
export INFORMIXDIR
INFORMIXSERVER=ejk_tcp
export INFORMIXSERVER
S4_RECEIVER_CONFIG=/usr/local/warez/projects/s4/siren/db/ReceiverConfig.tab
export S4_RECEIVER_CONFIG

PROJECTDIR=/disks/koloth/a/inet_services/boinc_www/share/projects/sah
SPLIT_PROG_LOC=${PROJECTDIR}/bin/
SPLIT_PROG_NAME=sah_splitter
SPLIT_TAPE_DIR=/disks/setifiler1/wutape/tapedir/seti_boinc_public/
SPLIT_SUFFIX=.split
#HIDONE_SUFFIX=.hidone
SPLITDONE_SUFFIX=.splitdone
EMAIL_LIST="korpela@ssl.berkeley.edu jeffc@ssl.berkeley.edu mattl@ssl.berkeley.edu davea@ssl.berkeley.edu"
FOUND_FILE=0
HOST=`hostname`
SPLIT_HALT_MSG=splitter_stop

# Check to see if this machine is already running the splitter program
PROG_RUNNING=`/usr/ucb/ps -auxww | grep "$SPLIT_PROG_NAME " | grep -v grep` 
if [ ! -z  "$PROG_RUNNING" ] 
then
  echo "Splitter is already running on this machine.  Quitting."
  exit 1
fi

# Get rid of all .split files here with HOSTNAME in them(?)

TAPE_FILE=

# get list of all tapes in the sah classic tape directory
cd $SPLIT_TAPE_DIR/..
FILE_LIST=`ls *.tape`

cd $SPLIT_TAPE_DIR

# if no sah classic tapes then bail
if [ -z "$FILE_LIST" ] 
then
    exit 2
fi

# for each sah classic tape...
for file in $FILE_LIST
do
  # if we have already split it...
  if [ -f $file$SPLITDONE_SUFFIX ]
  then
    #  ... then remove it ("it" being a hard link upward into the sah classic dir)
    if [ -f $file ]
    then
      /bin/rm $file
    fi
  else
    # if we are in the process of splitting it...
    if [ -f $file$SPLIT_SUFFIX ] 
    then
      # and are splitting it from this host...  (this logic prevents one host from repeating another host's tape)
      if grep $HOST $file$SPLIT_SUFFIX 
      then
        # trigger to restart the split
        /bin/rm $file$SPLIT_SUFFIX
      fi
    fi
    # if a restart or a brand new tape...
    if [ ! -f $file$SPLIT_SUFFIX ] 
    then
      # claim it for this host (there is certainly a race condition here)
      echo $HOST > $file$SPLIT_SUFFIX
      ln ../$file
      TAPE_FILE=$file
      SPLIT_FILE=$SPLIT_TAPE_DIR$file
      SPLIT_MARK_FILE=$SPLIT_TAPE_DIR$file$SPLIT_SUFFIX
#     HI_DONE_FILE=$SPLIT_TAPE_DIR$file$HIDONE_SUFFIX
      SPLIT_DONE_FILE=$SPLIT_TAPE_DIR$file$SPLITDONE_SUFFIX
      break
    fi
  fi
done

cd /tmp/splitter/sah
if [ ! -d splitter ]
then
  mkdir splitter
fi
cd splitter
if [ ! -d wu_inbox ]
then
  mkdir wu_inbox
fi
/bin/rm -f error.log rcd.chk
touch error.log
touch rcd.chk

if [ -z "$TAPE_FILE" ]
then
  exit 3
else
  /bin/rm -f wu_inbox/.*
  cat error.log rcd.chk | /usr/ucb/mail -s""$HOST" boinc_splitter starting" $EMAIL_LIST
  /bin/nice $SPLIT_PROG_LOC$SPLIT_PROG_NAME $SPLIT_FILE $* -resume -projectdir=$PROJECTDIR 
  if grep -i "end" error.log >/dev/null 2>&1
  then 
    /bin/nice $SPLIT_PROG_LOC$SPLIT_PROG_NAME $SPLIT_FILE $* -resume -projectdir=$PROJECTDIR
  fi
  if grep -i "end" error.log >/dev/null 2>&1
  then
    cat error.log rcd.chk | /usr/ucb/mail -s""$HOST" boinc_splitter finished" $EMAIL_LIST
    /bin/rm -f error.log rcd.chk
    /bin/rm -f wu_inbox/.*
    /bin/rm -f $SPLIT_MARK_FILE
    /bin/rm -f `cd $SPLIT_TAPE_DIR; ls -l | grep $TAPE_FILE | awk '{print $NF}'`
    /bin/rm -f $SPLIT_FILE
    touch -f $SPLIT_DONE_FILE
    exit
  else 
    /bin/rm -f $SPLIT_MARK_FILE
    exit 4
  fi
fi

#if ( 0 == 1 ) then
# if ( grep -i done hi_log ) then
#  cat error.log rcd.chk | /usr/ucb/mail -s"end of tape on $HOST" $EMAIL_LIST
#  touch $HI_FILE$HIDONE_SUFFIX
#  if ( -f $HI_FILE$HIDONE_SUFFIX ) then
# unlink $SPLITFILE
#  fi
# unlink .$SPLITFILE
#  /bin/rm error.log rcd.chk
#  /bin/rm wu_inbox/.*
# nice -10 $SPLITTER_PROG_LOC $SPLITFILE -resume 2>> splitterlog &
#else
#  if [ -f $MARKER$HOST ]
#  then
#    /bin/rm wu_inbox/.*
#    cat error.log rcd.chk | /usr/ucb/mail -s"restarting HI analysis on $HOST" $EMAIL_LIST
# nice -10 $SPLITTER_PROG_LOC $SPLITFILE -resume 2>> splitterlog &
#  else
#    cat error.log rcd.chk | /usr/ucb/mail -s"hi analysis disabled on $HOST" $EMAIL_LIST
#  fi
#endif
end

