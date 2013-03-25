AC_DEFUN([SAH_CHECK_MATH_FUNCS],[
  for func in $1 ; do
    AH_TEMPLATE(AS_TR_CPP(have_${func}),[Define to 1 if you have the function ${func}()])
    AC_CHECK_LIB([m],$func,[
      eval "ac_cv_func_${func}=yes"
      AC_DEFINE_UNQUOTED(AS_TR_CPP(have_${func}),1,[Define to 1 if you have the function ${func()}])
    ])
    AC_CHECK_FUNC($func)
  done
])
