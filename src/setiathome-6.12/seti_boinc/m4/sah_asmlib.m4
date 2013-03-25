AC_DEFUN([SAH_CHECK_ASMLIB],[
  AC_LANG_PUSH([C++])
  AC_MSG_CHECKING([asmlib])
  AC_ARG_ENABLE(asmlib,
    AC_HELP_STRING([--disable-asmlib],
      [disable use of asmlib for CPU identification (iX86 only)]),
    [
      disable_asmlib=yes
      enable_asmlib=no
    ],
    [
      disable_asmlib=no
      enable_asmlib=yes
    ]
  )
  if test -z "`echo $target | grep '[3456]86'`" ; then
    disable_asmlib=yes
    enable_asmlib=no
  fi
  ASMLIB_CFLAGS=
  ASMLIB_LDFLAGS=
  ASMLIB_LIBS=
  asmlib_save_CFLAGS="${CFLAGS}"
  asmlib_save_CXXFLAGS="${CXXFLAGS}"
  asmlib_save_LDFLAGS="${LDFLAGS}"
  asmlib_save_LIBS="${LIBS}"
  asmlib_dir="${SAH_TOP_DIR}/client/vector/"
  asmlib_works=no
  if test "${disable_asmlib}" != "yes" ; then
    CFLAGS="${CFLAGS} -I${asmlib_dir}"
    CXXFLAGS="${CXXFLAGS} -I${asmlib_dir}"
    for ASMLIB_LIBS in ${asmlib_dir}/asmlibe.a ${asmlib_dir}/asmlibm.a ${asmlib_dir}/asmlibo.lib
    do
      LIBS="${ASMLIB_LIBS} ${asmlib_save_LIBS}"
      AC_LINK_IFELSE(
        [
	  AC_LANG_PROGRAM(
	    [[#include "asmlib.h"]],
            [[InstructionSet()]]
	  )
        ],
        [asmlib_works=yes; break]
      )
    done
  fi
  if test "${asmlib_works}" = "yes" ; then
    AC_DEFINE_UNQUOTED([USE_ASMLIB],[1],
      [Define to 1 to use ASMLIB to determine processor capabilities])
    AC_MSG_RESULT([$ASMLIB_LIBS])
  else
    ASMLIB_LIBS=
    AC_MSG_RESULT([no])
  fi
  CFLAGS="${asmlib_save_CFLAGS}"
  CXXFLAGS="${asmlib_save_CXXFLAGS}"
  LDFLAGS="${asmlib_save_LDFLAGS}"
  LIBS="${asmlib_save_LIBS}"
  AC_SUBST([ASMLIB_LDFLAGS])
  AC_SUBST([ASMLIB_CFLAGS])
  AC_SUBST([ASMLIB_LIBS])
  AC_LANG_POP([C++])
])
