###################
# Makefile
# Author: Mateusz Szpakowski
########################

COMMON_DEFS = -DWITH_ERRLIST            \
	-DANDROID_CHANGES               \
	-DUSE_LOCKS                     \
	-DREALLOC_ZERO_BYTES_FREES      \
	-D_LIBC=1                       \
	-DSOFTFLOAT                     \
	-DFLOATING_POINT                \
	-DNEED_PSELECT=1                \
	-DINET6 \
	-I$(LOCAL_PATH)/private \
	-DUSE_DL_PREFIX \
	-DPOSIX_MISTAKE
 
DEFS=
#DEFS=-DDEBUG=1 
#DEFS=-DDEBUG=1 -DTESTING=1 -DSLEEPING=1
#DEFS=-DTESTING=1
#DEFS=

LIBEW = libexecwrapper.so

all: $(LIBEW)

clean:
	rm -f *.o bionic/*.o bionic/stdio/*.o $(LIBEW)

BIONICSRCS = bionic/creat.c bionic/exec.c bionic/open.c bionic/opendir.c \
	bionic/popen.c bionic/realpath.c bionic/utime.c bionic/system.c \
	bionic/strtod.c

BIONICOBJS = bionic/creat.o bionic/exec.o bionic/open.o bionic/opendir.o \
	bionic/popen.o bionic/realpath.o bionic/utime.o bionic/system.o \
	bionic/strtod.o

BIONICSTDSRCS = \
    bionic/stdio/asprintf.c \
    bionic/stdio/clrerr.c \
    bionic/stdio/fclose.c \
    bionic/stdio/fdopen.c \
    bionic/stdio/feof.c \
    bionic/stdio/ferror.c \
    bionic/stdio/fflush.c \
    bionic/stdio/fgetc.c \
    bionic/stdio/fgetln.c \
    bionic/stdio/fgetpos.c \
    bionic/stdio/fgets.c \
    bionic/stdio/fileno.c \
    bionic/stdio/findfp.c \
    bionic/stdio/flags.c \
    bionic/stdio/flockfile.c \
    bionic/stdio/fopen.c \
    bionic/stdio/fprintf.c \
    bionic/stdio/fpurge.c \
    bionic/stdio/fputc.c \
    bionic/stdio/fputs.c \
    bionic/stdio/fread.c \
    bionic/stdio/freopen.c \
    bionic/stdio/fscanf.c \
    bionic/stdio/fseek.c \
    bionic/stdio/fsetpos.c \
    bionic/stdio/ftell.c \
    bionic/stdio/funopen.c \
    bionic/stdio/fvwrite.c \
    bionic/stdio/fwalk.c \
    bionic/stdio/fwrite.c \
    bionic/stdio/getc.c \
    bionic/stdio/getchar.c \
    bionic/stdio/gets.c \
    bionic/stdio/makebuf.c \
    bionic/stdio/mktemp.c \
    bionic/stdio/printf.c \
    bionic/stdio/putc.c \
    bionic/stdio/putchar.c \
    bionic/stdio/puts.c \
    bionic/stdio/putw.c \
    bionic/stdio/refill.c \
    bionic/stdio/remove.c \
    bionic/stdio/rewind.c \
    bionic/stdio/rget.c \
    bionic/stdio/scanf.c \
    bionic/stdio/setbuf.c \
    bionic/stdio/setbuffer.c \
    bionic/stdio/setvbuf.c \
    bionic/stdio/snprintf.c \
    bionic/stdio/sprintf.c \
    bionic/stdio/sscanf.c \
    bionic/stdio/stdio.c \
    bionic/stdio/tempnam.c \
    bionic/stdio/tmpfile.c \
    bionic/stdio/tmpnam.c \
    bionic/stdio/ungetc.c \
    bionic/stdio/vasprintf.c \
    bionic/stdio/vfprintf.c \
    bionic/stdio/vfscanf.c \
    bionic/stdio/vprintf.c \
    bionic/stdio/vscanf.c \
    bionic/stdio/vsnprintf.c \
    bionic/stdio/vsprintf.c \
    bionic/stdio/vsscanf.c \
    bionic/stdio/wbuf.c \
    bionic/stdio/wsetup.c

BIONICSTDOBJS = \
    bionic/stdio/asprintf.o \
    bionic/stdio/clrerr.o \
    bionic/stdio/fclose.o \
    bionic/stdio/fdopen.o \
    bionic/stdio/feof.o \
    bionic/stdio/ferror.o \
    bionic/stdio/fflush.o \
    bionic/stdio/fgetc.o \
    bionic/stdio/fgetln.o \
    bionic/stdio/fgetpos.o \
    bionic/stdio/fgets.o \
    bionic/stdio/fileno.o \
    bionic/stdio/findfp.o \
    bionic/stdio/flags.o \
    bionic/stdio/flockfile.o \
    bionic/stdio/fopen.o \
    bionic/stdio/fprintf.o \
    bionic/stdio/fpurge.o \
    bionic/stdio/fputc.o \
    bionic/stdio/fputs.o \
    bionic/stdio/fread.o \
    bionic/stdio/freopen.o \
    bionic/stdio/fscanf.o \
    bionic/stdio/fseek.o \
    bionic/stdio/fsetpos.o \
    bionic/stdio/ftell.o \
    bionic/stdio/funopen.o \
    bionic/stdio/fvwrite.o \
    bionic/stdio/fwalk.o \
    bionic/stdio/fwrite.o \
    bionic/stdio/getc.o \
    bionic/stdio/getchar.o \
    bionic/stdio/gets.o \
    bionic/stdio/makebuf.o \
    bionic/stdio/mktemp.o \
    bionic/stdio/printf.o \
    bionic/stdio/putc.o \
    bionic/stdio/putchar.o \
    bionic/stdio/puts.o \
    bionic/stdio/putw.o \
    bionic/stdio/refill.o \
    bionic/stdio/remove.o \
    bionic/stdio/rewind.o \
    bionic/stdio/rget.o \
    bionic/stdio/scanf.o \
    bionic/stdio/setbuf.o \
    bionic/stdio/setbuffer.o \
    bionic/stdio/setvbuf.o \
    bionic/stdio/snprintf.o \
    bionic/stdio/sprintf.o \
    bionic/stdio/sscanf.o \
    bionic/stdio/stdio.o \
    bionic/stdio/tempnam.o \
    bionic/stdio/tmpfile.o \
    bionic/stdio/tmpnam.o \
    bionic/stdio/ungetc.o \
    bionic/stdio/vasprintf.o \
    bionic/stdio/vfprintf.o \
    bionic/stdio/vfscanf.o \
    bionic/stdio/vprintf.o \
    bionic/stdio/vscanf.o \
    bionic/stdio/vsnprintf.o \
    bionic/stdio/vsprintf.o \
    bionic/stdio/vsscanf.o \
    bionic/stdio/wbuf.o \
    bionic/stdio/wsetup.o

BIONICSTDHDRS = bionic/stdio/fileext.h \
	bionic/stdio/fvwrite.h \
	bionic/stdio/local.h \
	bionic/stdio/floatio.h \
	bionic/stdio/glue.h \
	bionic/stdio/wcio.h

LIBEWSRCS = execinit.c execwrapper.c execperms.c $(BIONICSRCS) $(BIONICSTDSRCS)
LIBEWOBJS = execinit.o execwrapper.o execperms.o $(BIONICOBJS) $(BIONICSTDOBJS)

$(LIBEW): $(LIBEWOBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -shared -o $@ $^

HDRS = config.h $(BIONICSTDHDRS)

SRCS = $(LIBEWSRCS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(COMMON_DEFS) $(DEFS) -c -o $@ $<

DEPS = make.deps

make.deps: $(SRCS) $(HDRS)
	$(CC) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -MM $(SRCS) > make.deps

include $(DEPS)
