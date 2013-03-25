# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_DEFUN([SAH_FIND_SETILIB],[
AC_MSG_CHECKING([path to setilib library])
setilib_search_path="$SETILIBDIR setilib ../setilib $HOME/setilib /usr/local /usr/local/setilib /usr/local/lib/setilib /opt/misc/setilib /opt/misc/lib/setilib"
for tmp_dir in $setilib_search_path
do
   if test -f $tmp_dir/lib/libseti.a
   then
     mydir=`pwd`
     cd $tmp_dir
     SETILIB_PATH=`pwd`
     SETILIB_LIBS="-L$SETILIB_PATH/lib -lseti"
     SETILIB_CFLAGS="-I$SETILIB_PATH/include -I$SETILIB_PATH"
     cd $mydir
     break
   fi
done
if test -n "$SETILIB_PATH" 
then
  AC_MSG_RESULT([$SETILIB_PATH])
else
  no_setilib=yes
  AC_MSG_RESULT([not found])
fi
AC_SUBST([SETILIB_PATH])
AC_SUBST([SETILIB_LIBS])
AC_SUBST([SETILIB_CFLAGS])
])

