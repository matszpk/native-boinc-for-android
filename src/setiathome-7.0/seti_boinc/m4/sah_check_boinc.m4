# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_PREREQ([2.54])

AC_DEFUN([SAH_CHECK_BOINC],[
  AC_ARG_VAR([BOINCDIR],[boinc directory])
  AC_ARG_VAR([PROJECTDIR],[project config.xml directory])
  if test -z "$HEAD"
  then
    AC_PATH_PROG(HEAD,head)
  fi
  if test -z "$FIND"
  then
    AC_PATH_PROG(FIND,find)
  fi
  thisdir=`pwd`
  AC_MSG_CHECKING([for BOINC])
  boinc_search_path="$BOINCDIR boinc ../boinc $HOME/boinc /usr/local/boinc /usr/local/lib/boinc /opt/misc/boinc /opt/misc/lib/boinc $2"
  for boinc_dir in $boinc_search_path
  do
    if test -d $boinc_dir 
    then
      if test -f $boinc_dir/include/std_fixes.h -o -f $boinc_dir/lib/std_fixes.h
      then
        cd $boinc_dir
        BOINCDIR=`pwd`
	cd $thisdir
	break
      else
        if $FIND $boinc_dir -name "std_fixes.h" >& /dev/null
	then
	  BOINCDIR=`$FIND $boinc_dir -name "std_fixes.h" -print | $HEAD -1 | sed 's/\/std_fixes.h//'`         
          cd $BOINCDIR/..
          BOINCDIR=`pwd`
	  cd $thisdir
	  break
	fi
      fi
    fi
  done
  if test -n "$BOINCDIR" 
  then
    AC_MSG_RESULT($BOINCDIR)
  else
    no_boinc=yes
    AC_MSG_RESULT(not found)
  fi
  if test -z "$PROJECTDIR"
  then
    PROJECTDIR=~boincadm/projects/ap
  fi
  AC_DEFINE_UNQUOTED([PROJECTDIR],["$PROJECTDIR"],[Define as directory containing the project config.xml])
  AC_SUBST([PROJECTDIR])
  AC_SUBST([BOINCDIR])
  BOINC_CFLAGS="-I$BOINCDIR -I$BOINCDIR/api -I$BOINCDIR/lib -I$BOINCDIR/sched -I$BOINCDIR/db"
  AC_SUBST([BOINC_CFLAGS])
])
	

