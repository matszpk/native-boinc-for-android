
AC_DEFUN([SAH_REQUIRES],[
   $2
   if test $3; then
     AC_MSG_WARN([ $1 not found.
============================================================================
$4
============================================================================
])
$5
   fi
])


AC_DEFUN([SAH_SERVER_REQUIRES],[
if test "${enable_server}" = yes; then
   SAH_REQUIRES([$1],[$2],[$3],
[
WARNING: trying to build the seti_boinc server but $1 was not found.
If you don't want to build the server you should use --disable-server.

I am continuing now as if --disable-server had been specified.
],[enable_server=no])
fi
])

AC_DEFUN([SAH_CLIENT_REQUIRES],[
if test "${enable_client}" = yes; then
   SAH_REQUIRES([$1],[$2],[$3],
[
WARNING: trying to build the seti_boinc client but $1 was not found.
If you don't want to build the client you should use --disable-client.

I am continuing now as if --disable-client had been specified.
],[enable_client=no])
fi
])

