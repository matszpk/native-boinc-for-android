
dnl $Id: sah_check_sah.m4,v 1.1 2006/10/19 00:14:01 korpela Exp $

AC_DEFUN([SAH_CHECK_SAH],[
AC_MSG_CHECKING(seti_boinc location)
[
mycwd=`pwd`
if [ -z "$SETI_BOINC_DIR" ]; then
   if [ -f ../seti_boinc/db/schema_to_class.in ]; then
      SETI_BOINC_DIR=`cd ${mycwd}/../seti_boinc && pwd`
   elif [ -f ../../seti_boinc/db/schema_to_class.in ]; then
      SETI_BOINC_DIR=`cd ${mycwd}/../../seti_boinc && pwd`
   elif [ -f ../../../seti_boinc/db/schema_to_class.in ]; then
      SETI_BOINC_DIR=`cd ${mycwd}/../../../seti_boinc && pwd`
   elif [ -f seti_boinc/db/schema_to_class.in ]; then
      SETI_BOINC_DIR=`cd seti_boinc && pwd`
   else
      echo "Couldn't find SETI_BOINC files; specify SETI_BOINC_DIR" >&2
      exit 1
   fi
fi
]
AC_MSG_RESULT($SETI_BOINC_DIR)
AC_SUBST(SETI_BOINC_DIR)
])
