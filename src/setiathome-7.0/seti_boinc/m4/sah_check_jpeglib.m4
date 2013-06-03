AC_DEFUN([SAH_CHECK_JPEGLIB],[
AC_ARG_WITH([my-libjpeg],
    AC_HELP_STRING([--with-my-libjpeg],[use the jpeglib that is distributed with setiatthome]),
    [use_my_libjpeg="$withval"])
if test "x${use_my_libjpeg}" = "xyes" ; then
  LIBS="-L`pwd`/jpeglib ${LIBS}"
  CFLAGS="${CFLAGS} -I`pwd`/jpeglib"
  sah_cv_lib_jpeg_jpeg_start_decompress="`pwd`/jpeglib/libjpeg.a"
  sah_cv_lib_jpeg_fopen="`pwd`/jpeglib/libjpeg.a"
  sah_cv_static_lib_jpeg_jpeg_start_decompress="$sah_cv_lib_jpeg_jpeg_start_decompress"
  sah_cv_static_lib_jpeg_fopen="$sah_cv_lib_jpeg_fopen"
  if test -f "${sah_cv_lib_jpeg_fopen}" ; then
   /bin/rm -f "${sah_cv_lib_jpeg_fopen}"
  fi
  echo "int foo() { return 0; }" >foo.c
  $CC -c foo.c
  $AR cr "${sah_cv_lib_jpeg_fopen}" foo.$OBJEXT
  /bin/rm foo.$OBJEXT foo.c
  $RANLIB "${sah_cv_lib_jpeg_fopen}"
fi
SAH_CHECK_LIB([jpeg], [jpeg_start_decompress],
    AC_DEFINE([HAVE_LIBJPEG],[1],[Define to 1 if you have the jpeg library]))
AM_CONDITIONAL(USE_MY_LIBJPEG, [test "x${use_my_libjpeg}" = "xyes" -o "${sah_cv_lib_jpeg_jpeg_start_decompress}" = "no"])
])
