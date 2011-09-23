/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */


/* double-inclusion protection for config.h */
#ifndef BOINC_CONFIG_H
#define BOINC_CONFIG_H

/* Version defines are now in version.h */
#include "version.h"  



/* Directory where BOINC executables will be installed */
#define BOINC_EXECPREFIX "/data/local/tmp/bin"

/* Directory where BOINC libraries will be installed */
#define BOINC_LIBDIR "/data/local/tmp/lib"

/* Directory tree where BOINC will be installed */
#define BOINC_PREFIX "/data/local/tmp"

/* Define to the type pointed to by the 5th parameter of getsockopt */
#define BOINC_SOCKLEN_T socklen_t

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if CYGWIN should use the WIN32 GUI */
/* #undef CYGWIN_USE_WIN32 */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if compiling under OS X 10.3 or earlier */
/* #undef DARWIN_10_3 */

/* Define to the platform's shared library extension */
#define DLLEXT ".so"

/* Define to 1 if you have the `alloca' function. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Use the Apple OpenGL framework. */
/* #undef HAVE_APPLE_OPENGL_FRAMEWORK */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `daemon' function. */
#define HAVE_DAEMON 1

/* Define to 1 if you have the declaration of `tzname', and to 0 if you don't.
   */
/* #undef HAVE_DECL_TZNAME */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the `ether_ntoa' function. */
/* #undef HAVE_ETHER_NTOA */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getisax' function. */
/* #undef HAVE_GETISAX */

/* Define to 1 if you have the `getutent' function. */
#define HAVE_GETUTENT 1

/* Define to 1 if you have the <glaux.h> header file. */
/* #undef HAVE_GLAUX_H */

/* Define to 1 if you have the <GLUT/glut.h> header file. */
/* #undef HAVE_GLUT_GLUT_H */

/* Define to 1 if you have the <glut.h> header file. */
/* #undef HAVE_GLUT_H */

/* Define to 1 if you have the <glu.h> header file. */
/* #undef HAVE_GLU_H */

/* Define to 1 if you have the <GL/glaux.h> header file. */
/* #undef HAVE_GL_GLAUX_H */

/* Define to 1 if you have the <GL/glut.h> header file. */
/* #undef HAVE_GL_GLUT_H */

/* Define to 1 if you have the <GL/glu.h> header file. */
/* #undef HAVE_GL_GLU_H */

/* Define to 1 if you have the <GL/gl.h> header file. */
/* #undef HAVE_GL_GL_H */

/* Define to 1 if you have the <gl.h> header file. */
/* #undef HAVE_GL_H */

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the <ieeefp.h> header file. */
/* #undef HAVE_IEEEFP_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the c library */
/* #undef HAVE_LIBC */

/* Define to 1 if you have the c_nonshared library */
/* #undef HAVE_LIBC_NONSHARED */

/* Define to 1 if you have the gcc library */
/* #undef HAVE_LIBGCC */

/* Define to 1 if you have the gcc_eh library */
/* #undef HAVE_LIBGCC_EH */

/* Define to 1 if you have the math library */
#define HAVE_LIBM 1

/* Define to 1 if you have the stdc++ library */
/* #undef HAVE_LIBSTDC__ */

/* Define to 1 if you have the wsock32 library */
/* #undef HAVE_LIBWSOCK32 */

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <MesaGL/glaux.h> header file. */
/* #undef HAVE_MESAGL_GLAUX_H */

/* Define to 1 if you have the <MesaGL/glut.h> header file. */
/* #undef HAVE_MESAGL_GLUT_H */

/* Define to 1 if you have the <MesaGL/glu.h> header file. */
/* #undef HAVE_MESAGL_GLU_H */

/* Define to 1 if you have the <MesaGL/gl.h> header file. */
/* #undef HAVE_MESAGL_GL_H */

/* Define if your C++ compiler supports namespaces */
#define HAVE_NAMESPACES 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/ether.h> header file. */
#define HAVE_NETINET_ETHER_H 1

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
#define HAVE_NETINET_IF_ETHER_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the <net/if_arp.h> header file. */
#define HAVE_NET_IF_ARP_H 1

/* Define to 1 if you have the <net/if.h> header file. */
#define HAVE_NET_IF_H 1

/* Define to 1 if you have the <OpenGL/glaux.h> header file. */
/* #undef HAVE_OPENGL_GLAUX_H */

/* Define to 1 if you have the <OpenGL/glut.h> header file. */
/* #undef HAVE_OPENGL_GLUT_H */

/* Define to 1 if you have the <OpenGL/glu.h> header file. */
/* #undef HAVE_OPENGL_GLU_H */

/* Define to 1 if you have the <OpenGL/gl.h> header file. */
/* #undef HAVE_OPENGL_GL_H */

/* Define to 1 if you have the <procfs.h> header file. */
/* #undef HAVE_PROCFS_H */

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the `putenv' function. */
#define HAVE_PUTENV 1

/* Define to 1 if you have the resolv library */
/* #undef HAVE_RESOLV */

/* Define to 1 if you have the <resolv.h> header file. */
#define HAVE_RESOLV_H 1

/* Define to 1 if you have the `res_init' function. */
#define HAVE_RES_INIT 1

/* Define to 1 if you have the `sched_setscheduler' function. */
#define HAVE_SCHED_SETSCHEDULER 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the <setjmp.h> header file. */
#define HAVE_SETJMP_H 1

/* Define to 1 if you have the `setpriority' function. */
#define HAVE_SETPRIORITY 1

/* Define to 1 if you have the `setutent' function. */
#define HAVE_SETUTENT 1

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have sqlite3. */
/* #undef HAVE_SQLITE3 */

/* Define to 1 if you have the `sqlite3_open' function. */
/* #undef HAVE_SQLITE3_OPEN */

/* Define to 1 if you have the `stat64' function. */
/* #undef HAVE_STAT64 */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if locale is in namespace std:: */
/* #undef HAVE_STD_LOCALE */

/* Define to 1 if max is in namespace std:: */
/* #undef HAVE_STD_MAX */

/* Define to 1 if min is in namespace std:: */
/* #undef HAVE_STD_MIN */

/* Define to 1 if transform is in namespace std:: */
/* #undef HAVE_STD_TRANSFORM */

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strcasestr' function. */
#define HAVE_STRCASESTR 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strdupa' function. */
/* #undef HAVE_STRDUPA */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcat' function. */
#define HAVE_STRLCAT 1

/* Define to 1 if you have the `strlcpy' function. */
#define HAVE_STRLCPY 1

/* Define to 1 if the system has the type `struct ether_addr'. */
#define HAVE_STRUCT_ETHER_ADDR 1

/* Define to 1 if the system has the type `struct ifconf'. */
#define HAVE_STRUCT_IFCONF 1

/* Define to 1 if the system has the type `struct ifreq'. */
#define HAVE_STRUCT_IFREQ 1

/* Define to 1 if the system has the type `struct lifconf'. */
/* #undef HAVE_STRUCT_LIFCONF */

/* Define to 1 if the system has the type `struct lifreq'. */
/* #undef HAVE_STRUCT_LIFREQ */

/* Define to 1 if `tm_zone' is a member of `struct tm'. */
#define HAVE_STRUCT_TM_TM_ZONE 1

/* Define to 1 if you have the <sys/auxv.h> header file. */
/* #undef HAVE_SYS_AUXV_H */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/fcntl.h> header file. */
/* #undef HAVE_SYS_FCNTL_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/ipc.h> header file. */
#define HAVE_SYS_IPC_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/msg.h> header file. */
/* #undef HAVE_SYS_MSG_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/shm.h> header file. */
/* #undef HAVE_SYS_SHM_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/statfs.h> header file. */
#define HAVE_SYS_STATFS_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
/* #undef HAVE_SYS_STATVFS_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/swap.h> header file. */
/* #undef HAVE_SYS_SWAP_H */

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

/* Define to 1 if you have the <sys/systeminfo.h> header file. */
/* #undef HAVE_SYS_SYSTEMINFO_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if you have the <sys/vmmeter.h> header file. */
/* #undef HAVE_SYS_VMMETER_H */

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if your `struct tm' has `tm_zone'. Deprecated, use
   `HAVE_STRUCT_TM_TM_ZONE' instead. */
#define HAVE_TM_ZONE 1

/* Define to 1 if you don't have `tm_zone' but do have the external array
   `tzname'. */
/* #undef HAVE_TZNAME */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
#define HAVE_UNSETENV 1

/* Define to 1 if you have the <utmp.h> header file. */
#define HAVE_UTMP_H 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to 1 if you have the <winsock2.h> header file. */
/* #undef HAVE_WINSOCK2_H */

/* Define to 1 if you have the <winsock.h> header file. */
/* #undef HAVE_WINSOCK_H */

/* Define to 1 if you have the `_alloca' function. */
/* #undef HAVE__ALLOCA */

/* Define to 1 if /proc/meminfo exists */
#define HAVE__PROC_MEMINFO 1

/* Define to 1 if /proc/self/psinfo exists */
/* #undef HAVE__PROC_SELF_PSINFO */

/* Define to 1 if /proc/self/stat exists */
#define HAVE__PROC_SELF_STAT 1

/* Define to 1 if you have the `__builtin_alloca' function. */
/* #undef HAVE___BUILTIN_ALLOCA */

/* Platform identification used to identify applications for this BOINC core
   client */
#define HOSTTYPE "arm-unknown-linux-gnu"

/* Alternate identification used to identify applications for this BOINC core
   client */
/* #undef HOSTTYPEALT */

/* "Define to 1 if largefile support causes missing symbols in C++" */
#define LARGEFILE_BREAKS_CXX 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "boinc"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "BOINC"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "BOINC 6.12.38"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "boinc"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "6.12.38"

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* "Define to 1 if you want to use the openssl crypto library" */
#define USE_OPENSSL 1

/* Define to 1 to use windows sockets */
/* #undef USE_WINSOCK */

/* Define to 1 to use the wxWidgets debug libraries */
/* #undef USE_WX_DEBUG */

/* utmp file location */
#define UTMP_LOCATION "/var/run/utmp"

/* Version number of package */
#define VERSION "6.12.38"

/* Define to prevent redefinition of INT32 in jconfig.h */
/* #undef XMD_H */

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */


#ifndef HAVE_SOCKLEN_T
typedef BOINC_SOCKLEN_T socklen_t;
#endif




#ifndef HAVE_RES_INIT
#define res_init() (0)
#endif

/* end double-inclusion protection for config.h */
#endif /* #ifndef BOINC_CONFIG_H */

