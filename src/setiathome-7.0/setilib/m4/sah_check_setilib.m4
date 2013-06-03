# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_PREREQ([2.54])

AC_DEFUN([SAH_CHECK_SETILIB],[
  AC_ARG_VAR([SETILIBDIR],[setilib directory])
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
  AC_MSG_CHECKING([for SETILIB])
  setilib_search_path="$SETILIBDIR setilib ../setilib $HOME/setilib /usr/local/setilib /usr/local/lib/setilib /opt/misc/setilib /opt/misc/lib/setilib $2"
  for setilib_dir in $setilib_search_path
  do
    if test -d $setilib_dir 
    then
      if test -f $setilib_dir/lib/Makefile
      then
        cd $setilib_dir
        SETILIBDIR=`pwd`
	cd $thisdir
	break
      else
        if $FIND $setilib_dir -name "lib/Makefile" >& /dev/null
	then
	  SETILIBDIR=`$FIND $setilib_dir -name "lib/Makefile" -print | $HEAD -1 | sed 's/\/lib/Makefile//'`         
          cd $SETILIBDIR
          SETILIBDIR=`pwd`
	  cd $thisdir
	  break
	fi
      fi
    fi
  done
  if test -n "$SETILIBDIR" 
  then
    AC_MSG_RESULT($SETILIBDIR)
  else
    AC_MSG_RESULT(not found)
  fi
])
	

