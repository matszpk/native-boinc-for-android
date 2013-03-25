# under construction - jeffc


EXEEXT = 
OBJEXT = o
LIBEXT = a
DLLEXT = so
DOTEXEEXT = 

SUFFIXES = .cpp .c .o .so .a 

BOINCDIR = /home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38

CC = /home/mat/docs/dev/mobile/android-ndk-r8b/toolchains/arm-linux-androideabi-4.7.2/prebuilt/linux-x86/bin//arm-linux-androideabi-gcc 
CFLAGS = --sysroot=/home/mat/docs/dev/mobile/android-ndk-r8b/platforms/android-8/arch-arm/ -Wall -O3 -funroll-loops -mhard-float -mfpu=vfp -mfloat-abi=softfp -fomit-frame-pointer -DANDROID=1 -I/home/mat/docs/dev/mobile/android-ndk-r8b/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include/ -I/home/mat/docs/dev/mobile/android-ndk-r8b/sources/cxx-stl/gnu-libstdc++/4.6/include/ -I/home/mat/docs/src/android/installdir/include -DHAVE_CONFIG_H -DTEXT_UI -DNDEBUG  

CXX = /home/mat/docs/dev/mobile/android-ndk-r8b/toolchains/arm-linux-androideabi-4.7.2/prebuilt/linux-x86/bin//arm-linux-androideabi-g++ 
CXXFLAGS = $(CFLAGS) 

LDFLAGS = -L/home/mat/docs/dev/mobile/android-ndk-r8b/platforms/android-8/arch-arm//usr/lib -L/home/mat/docs/src/android/installdir/lib 

CLIBS =  

#SUFFIXES = .cpp .o

#BOINC_INC	= -I$(BOINCDIR)/api -I$(BOINCDIR)/lib -I$(BOINCDIR)/sched -I$(BOINCDIR)/db
BOINCDIR        = /home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38
BOINC_CFLAGS    = -I/home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38 -I/home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38/api -I/home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38/lib -I/home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38/sched -I/home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38/db
BOINC_LIBS 	    = -L$(BOINCDIR)/sched -lsched  -L$(BOINCDIR)/lib -lboinc
MYSQL_CFLAGS    = 
MYSQL_LIBS      = 
INFORMIXDIR     = 
INFORMIX_CFLAGS = 
INFORMIX_LIBS   = 

OBJS = \
    sah_validate.$(OBJEXT) \
    sah_result.$(OBJEXT) \
    sah_boinc_db.$(OBJEXT)

BOINC_OBJS = $(BOINCDIR)/sched/validator.o \
        $(BOINCDIR)/sched/validate_util.o 


PROG = sah_validate

.cpp.o:
	$(CXX) $(CXXFLAGS) $(BOINC_CFLAGS) $(MYSQL_CFLAGS) -I.. -I../db -c -o $*.o $<

.c.o:
	$(CC) $(CFLAGS) $(BOINC_CFLAGS) -I.. -I../db -c -o $*.o $<

#all: Makefile dependencies $(PROGS)
all: Makefile $(PROG)

../aclocal.m4: ../m4/*.m4
	/usr/bin/cat ../m4/*.m4 >../aclocal.m4

../configure: ../configure.ac ../aclocal.m4 ../config.h.in
	(cd ..; make config.h)

$(PROG): $(OBJS) $(BOINC_OBJS)
	$(CXX) $(OBJS) $(BOINC_OBJS) -I../db $(BOINC_CFLAGS) $(CLIBS) $(BOINC_LIBS) $(MYSQL_LIBS) $(INFORMIX_LIBS) -o $(PROG)

clean:
	rm -f *.$(OBJEXT) $(PROG) dependencies config.log config.cache

#include dependencies
