AC_DEFUN([SAH_OPTIMIZATIONS],[
if test x_$sah_opt_only_once = x_ ; then

sah_opt_only_once=all_done
   
AC_ARG_ENABLE([sse3],
    AC_HELP_STRING([--enable-sse3],
       [Use SSE3 optimizations])
)

if test x_$enable_sse3 = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_SSE3],[1],
    [Define to 1 if you want to use SSE3 optimizations])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    CFLAGS="-march=prescott -msse3  -mfpmath=sse ${CFLAGS}"
  fi
fi

AC_ARG_ENABLE([sse2],
    AC_HELP_STRING([--enable-sse2],
       [Use SSE2 optimizations])
)

if test x_$enable_sse2 = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_SSE2],[1],
    [Define to 1 if you want to use SSE2 optimizations])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    CFLAGS="-msse2 -mfpmath=sse ${CFLAGS}"
    if test -z "echo $CFLAGS | grep march=" ; then
      CFLAGS="-march=pentium4 ${CFLAGS}" 
    fi
  fi
fi

AC_ARG_ENABLE([sse],
    AC_HELP_STRING([--enable-sse],
       [Use SSE optimizations])
)

if test x_$enable_sse = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_SSE],[1],
    [Define to 1 if you want to use SSE optimizations])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    CFLAGS="-march=pentium3 -msse -mfpmath=sse ${CFLAGS}"
  fi
fi

AC_ARG_ENABLE([mmx],
    AC_HELP_STRING([--enable-mmx],
       [Use MMX optimizations])
)

if test x_$enable_mmx = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_MMX],[1],
    [Define to 1 if you want to use MMX optimizations])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    CFLAGS="-march=pentium2 -mmmx -mfpmath=387 ${CFLAGS}"
  fi
fi

AC_ARG_ENABLE([3dnow],
    AC_HELP_STRING([--enable-3dnow],
       [Use 3dnow optimizations])
)


if test x_$enable_3dnow = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_3DNOW],[1],
    [Define to 1 if you want to use 3D-Now optimizations])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    CFLAGS="-march=pentium2 -m3dnow -mfpmath=387 ${CFLAGS}"
  fi
fi

AC_ARG_ENABLE([fast-math],
    AC_HELP_STRING([--enable-fast-math],
       [Use gcc -ffast-math optimization])
       )

if test x_$enable_fast_math = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_FAST_MATH],[1],
    [Define to 1 if you want to use the gcc -ffast-math optimization])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    CFLAGS="${CFLAGS} -ffast-math"
  fi
fi

AC_ARG_ENABLE([altivec],
    AC_HELP_STRING([--enable-altivec],
       [Use altivec optimizations])
       )

if test x_$enable_altivec = x_yes ; then
  AC_DEFINE_UNQUOTED([USE_ALTIVEC],[1],
    [Define to 1 if you want to use ALTIVEC optimizations])
# put compiler specific flags here
  if test x_$ac_cv_c_compiler_gnu = x_yes ; then
    SAH_CHECK_CFLAG([-faltivec],[CFLAGS="-faltivec ${CFLAGS}"])
    SAH_CHECK_CFLAG([-maltivec],[CFLAGS="-maltivec ${CFLAGS}"])
    SAH_CHECK_CFLAG([-mtune=G5],[CFLAGS="-mtune=G5 ${CFLAGS}"])
    SAH_CHECK_CFLAG([-mcpu=powerpc],[CFLAGS="-mcpu=powerpc ${CFLAGS}"])
    SAH_CHECK_LDFLAG([-framework Accelerate],[LDFLAGS="${LDFLAGS} -framework Accelerate"])
  fi
fi

fi
])
