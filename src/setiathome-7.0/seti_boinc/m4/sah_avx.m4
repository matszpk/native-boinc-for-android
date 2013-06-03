AC_DEFUN([SAH_AVX],[
  AC_LANG_PUSH(C)
  AC_MSG_CHECKING([if compiler supports -mavx])
  save_cflags="${CFLAGS}"
  CFLAGS="-mavx"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[return 0;])],[
  have_avx=yes
  ],[
  have_avx=no
  ]
  )
  AC_MSG_RESULT($have_avx)
  if test "$have_avx" = "yes" ; then
    AC_MSG_CHECKING([type of arg 2 of _mm256_maskstore_ps()])
    for type in __m256d __m256i __m256 ; do
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_IMMINTRIN_H)
#include <immintrin.h> 
#elif defined(HAVE_AVXINTRIN_H)
#include <avxintrin.h> 
#elif defined(HAVE_X86INTRIN_H)
#include <x86intrin.h> 
#elif defined(HAVE_INTRIN_H)
#include <intrin.h> 
#endif 

float *a1; 
$type  a2; 
__m256  a3; 
int foo(void) {
    _mm256_maskstore_ps(a1,a2,a3);
    return 0;
}
]],[return foo();])],
[ type_found=yes ],
[ type_found=no ])
       if test "${type_found}" = "yes" ; then
         break;
       fi
    done
  fi
  if test "${type_found}" = "yes" ; then
    AC_MSG_RESULT($type)
    avx_type=${type}
  else
    AC_MSG_RESULT(none)
    avx_type=
  fi
  CFLAGS="${save_cflags}"
  AC_LANG_POP(C)
])
