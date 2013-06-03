# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_DEFUN([SAH_CHECK_INFORMIX],[
  AC_ARG_VAR([INFORMIXDIR],[informix directory])
  AC_LANG_PUSH(C)
  AC_MSG_CHECKING(for Informix)
  if test -z "$INFORMIXDIR"  
  then
    AC_PATH_PROG([INFORMIXDIR],[dbaccess],[],[/disks/galileo/a/apps/informix/bin:$PATH:/usr/local/informix/bin:/opt/misc/informix:~informix/bin])
    INFORMIXDIR=`echo $INFORMIXDIR | $SED 's/bin\/dbaccess//'`
  fi
  if test -n "$INFORMIXDIR" 
  then
    AC_MSG_RESULT(yes)
    INFORMIX_CFLAGS="-I$INFORMIXDIR/incl -I$INFORMIXDIR/incl/esql "
    INFORMIX_LIBS=" -L$INFORMIXDIR/lib -L$INFORMIXDIR/lib/esql $INFORMIXDIR/lib/esql/checkapi.o -lifasf -lifgen -lifgls -lifos -lifsql"
    AC_CHECK_LIB([socket], [bind], INFORMIX_LIBS="${INFORMIX_LIBS} -lsocket")
    AC_CHECK_LIB([nsl], [gethostbyname], INFORMIX_LIBS="${INFORMIX_LIBS} -lnsl")
    AC_CHECK_LIB([crypt], [crypt], INFORMIX_LIBS="${INFORMIX_LIBS} -lcrypt")
    AC_CHECK_LIB([dl], [dlopen], INFORMIX_LIBS="${INFORMIX_LIBS} -ldl")
    AC_CHECK_LIB([m], [ceil], INFORMIX_LIBS="${INFORMIX_LIBS} -lm")
    
    tmpvar=$LIBS
    LIBS=`echo $LIBS $INFORMIX_LIBS`
    AC_MSG_CHECKING(informix library flags)
    AC_TRY_LINK_FUNC(main,AC_DEFINE([USE_INFORMIX],[1],[Define to 1 if informix is installed]) sah_cv_use_informix="yes", sah_cv_use_informix="no")
    AC_MSG_RESULT($sah_cv_use_informix)
    LIBS=$tmpvar
    SAH_FORCE_LIBSEARCH_PATH(tmpvar,[$INFORMIXDIR/lib:$INFORMIXDIR/lib/esql])
    INFORMIX_LIBS=`echo $tmpval $INFORMIX_LIBS`
  else
    INFORMIXDIR=
    INFORMIX_CFLAGS=
    INFORMIX_LIBS=
    AC_MSG_RESULT(no)
    no_informix=yes
  fi
  AC_SUBST(INFORMIXDIR)
  AC_SUBST(INFORMIX_CFLAGS)
  AC_SUBST(INFORMIX_LIBS)
  AC_LANG_POP
])


