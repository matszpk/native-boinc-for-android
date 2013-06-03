

AC_DEFUN([SAH_GRX_LIBS],[
  AC_LANG_PUSH(C)
  sah_save_libs="$LIBS"
  AC_PATH_PROG(XLSFONTS,[xlsfonts],[/usr/X11/bin:/usr/X11R6/bin:/usr/X11R5/bin:/usr/X11R4/bin:/usr/openwin/bin:$PATH]) 
  sah_xpath=`echo $XLSFONTS | sed 's/bin\/xlsfonts//' `
  LIBS=`echo $LIBS "-L$sah_xpath"`
  GRXLIBS="-L${sah_xpath} -ljpeg -lX11 -lXext -lXt -lICE -lSM -lGL -lGLU -lm"
  LIBS="$sah_save_libs"
  AC_SUBST(GRXLIBS)
  AC_LANG_POP
])

AC_DEFUN([SAH_GRX_INCLUDES],[
  AC_LANG_PUSH(C++)
  AC_PATH_PROG(XLSFONTS,[xlsfonts],[/usr/X11/bin:/usr/X11R6/bin:/usr/X11R5/bin:/usr/X11R4/bin:/usr/openwin/bin:$PATH]) 
  sah_xpath=`echo $XLSFONTS | sed 's/bin\/xlsfonts//' `
  save_cflags="$CFLAGS"
  CFLAGS=`echo $CFLAGS "-I$sah_xpath/include"`
  GRX_CFLAGS="-I$sah_xpath"
  AC_SUBST(GRX_CFLAGS)
  AC_CHECK_HEADERS([gl.h glu.h glut.h glaux.h GL/gl.h GL/glu.h GL/glut.h GL/glaux.h OpenGL/gl.h OpenGL/glu.h OpenGL/glut.h OpenGL/glaux.h glut/glut.h MesaGL/gl.h MesaGL/glu.h MesaGL/glut.h MesaGL/glaux.h])
  CFLAGS="$save_cflags"
  AC_LANG_POP
])

