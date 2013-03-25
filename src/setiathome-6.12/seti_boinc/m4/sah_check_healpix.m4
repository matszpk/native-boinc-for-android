# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_PREREQ([2.54])

AC_DEFUN([SAH_CHECK_HEALPIX],[
  AC_ARG_VAR([HEALPIX],[HEALPIX directory])
  if test -z "$HEAD"
  then
    AC_PATH_PROG(HEAD,head)
  fi
  if test -z "$FIND"
  then
    AC_PATH_PROG(FIND,find)
  fi
  thisdir=`pwd`
  AC_MSG_CHECKING([for HEALPIX])
  healpix_search_path=[`echo $HEALPIX [Hh]ealpix* ../[Hh]ealpix* ../lib/[Hh]ealpix* $HOME/lib/[Hh]ealpix* /usr/local/[Hh]ealpix* /usr/local`]
  for healpix_dir in $healpix_search_path
  do
    if test -d $healpix_dir/lib 
    then
      if test -f $healpix_dir/lib/libchealpix.a
      then
        cd $healpix_dir
        HEALPIX=`pwd`
	cd $thisdir
	break
      else
        if $FIND $healpix_dir -name "lib/libchealpix.a" 2>/dev/null >/dev/null
	then
	  HEALPIX=`$FIND $healpix_dir -name "lib/libchealpix.a" -print | $HEAD -1 | sed 's/\/lib\/libchealpix.a//'`         
          cd $HEALPIX
          HEALPIX=`pwd`
	  cd $thisdir
	  break
	fi
      fi
    fi
  done
  if test -n "$HEALPIX" 
  then
    AC_MSG_RESULT($HEALPIX)
  else
    AC_MSG_RESULT(not found)
    no_healpix=yes
  fi
])
	

