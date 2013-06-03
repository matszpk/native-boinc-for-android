# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_DEFUN([SAH_FORCE_LIBSEARCH_PATH],[
  if test -z "$LD" 
  then
    AC_PATH_PROGS([LD],[ld link])
  fi
  if test "$GCC" = "yes" 
  then 
    link_pass_flag="-Xlinker "
  else
    if test "$CC" = "lcc"
    then
      link_pass_flag="-Wl"
    fi
  fi
  if test -n "$link_pass_flag"
  then
    if test -n "$LD"
    then
      if $LD -V  2>&1 | grep GNU
      then
         $1="$link_pass_flag -rpath $link_pass_flag $2 "
      else
         $1="$link_pass_flag -R $link_pass_flag $2 "
      fi
    fi
  fi
])


