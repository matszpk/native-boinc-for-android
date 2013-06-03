AC_DEFUN([SAH_RAW_LDFLAGS],[
  $2=`echo $1 | $SED 's/-Wl,//g'`
  echo RAW_LDFLAGS:$2=`echo $1 | $SED 's/-Wl,//g'`  >&5
  AC_SUBST([$2])
])
