# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_DEFUN([SAH_FIND_S4PATH],[
AC_MSG_CHECKING([path to S4 library])
s4_search_path="$S4PATH /usr/local/warez/projects/s4/siren /disks/cyclops/c/users/seti/s4/siren"
for tmp_dir in $s4_search_path
do
   if test -f $tmp_dir/lib/libs4.a
   then
     S4PATH=$tmp_dir
     S4LIBS="-L$S4PATH/lib -ls4"
     S4CFLAGS="-I$S4PATH/include"
     S4_RECEIVER_CONFIG="$S4PATH/db/ReceiverConfig.tab"
     break
   fi
done
if test -n "$S4PATH" 
then
  AC_MSG_RESULT([$S4PATH])
else
  no_s4=yes
  AC_MSG_RESULT([not found])
fi
AC_SUBST([S4PATH])
AC_SUBST([S4LIBS])
AC_SUBST([S4CFLAGS])
AC_DEFINE_UNQUOTED([S4_RECEIVER_CONFIG_FILE],["$S4_RECEIVER_CONFIG"],[Path to the S4 receiver config file])
])

