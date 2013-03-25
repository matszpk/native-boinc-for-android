AC_DEFUN([SAH_CHECK_IPP],
[AC_MSG_CHECKING([for Intel Performance Primitives])
IPP=
found_ipp="no"
AC_ARG_WITH(ipp,
    AC_HELP_STRING([--with-ipp@<:@=DIR@:>@],
       [Use Intel Performance Primitives (in specified installation directory)]),
    [check_ipp_dir="$withval"],
    [check_ipp_dir=])

SAH_OPTIMIZATIONS

for dir in $check_ipp_dir $check_ipp_dir/ia32* $check_ipp_dir/ipp*/ia32* /opt/intel/ipp*/ia32* /usr/local/ipp*/ia32* /usr/lib/ipp*/ia32* /usr/ipp*/ia32* /usr/pkg/ipp*/ia32* /usr/local /usr; do
   ippdir="$dir"
   if test -f "$dir/include/ipp.h"; then
     found_ipp="yes";
     IPPDIR="${ippdir}"
     CFLAGS="$CFLAGS -I$ippdir/include -I$ippdir/tools/staticlib";
     CXXFLAGS="$CXXFLAGS -I$ippdir/include -I$ippdir/tools/staticlib";
     break;
   fi
   if test -f "$dir/include/ipp.h"; then
     found_ipp="yes";
     IPPDIR="${ippdir}"
     CFLAGS="$CFLAGS -I$ippdir/include/";
     CXXFLAGS="$CXXFLAGS -I$ippdir/include/";
     break
   fi
done
AC_MSG_RESULT($found_ipp)
if test x_$found_ipp = x_yes ; then
        printf "IPP found in $ippdir\n";
        LIBS="$LIBS -lippcore -lippsmerged";
        LDFLAGS="$LDFLAGS -L$ippdir/lib";
	AC_DEFINE_UNQUOTED([USE_IPP],[1],
	  ["Define to 1 if you want to use the Intel Performance Primitives"])
	AC_SUBST(IPPDIR)
fi
])
