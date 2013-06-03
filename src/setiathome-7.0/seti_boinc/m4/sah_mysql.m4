# SETI_BOINC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.

AC_DEFUN([SAH_CHECK_MYSQL],[
AC_LANG_PUSH(C)
AC_ARG_VAR([MYSQL_CONFIG], [mysql_config program])
if test -z "$MYSQL_CONFIG"; then
    AC_PATH_PROG(MYSQL_CONFIG,mysql_config,,[$PATH:/usr/local/mysql/bin:/opt/misc/mysql/bin:~mysql/bin])
fi
if test -z "$MYSQL_CONFIG"
then
    echo MYSQL not found
    no_mysql=yes
else
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING(mysql libraries)
    MYSQL_LIBS=`${MYSQL_CONFIG} --libs`
    AC_MSG_RESULT($MYSQL_LIBS)
    AC_MSG_CHECKING(mysql includes)
    MYSQL_CFLAGS=`${MYSQL_CONFIG} --cflags`
    AC_DEFINE(USE_MYSQL,1,[Define if MYSQL is installed])
    AC_MSG_RESULT($MYSQL_CFLAGS)
fi
AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_CFLAGS)
AC_LANG_POP
])

