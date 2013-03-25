# $Id: Makefile.in,v 1.14.2.1 2007/03/13 17:41:27 vonkorff Exp $
#
#



EXEEXT = 
OBJEXT = o
LIBEXT = a
DLLEXT = so
DOTEXEEXT = 


SUFFIXES = .ec .cpp .c .o .so .a 

BOINCDIR = /home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38
INFORMIXDIR = 
MYSQL_LIBS = 
MYSQL_CFLAGS = 
INFORMIX_CFLAGS = 

DBLIBS=  -lsocket  -lm -lstdc++ 

CC = /home/mat/docs/dev/mobile/android-ndk-r8b/toolchains/arm-linux-androideabi-4.7.2/prebuilt/linux-x86/bin//arm-linux-androideabi-gcc
CXX = /home/mat/docs/dev/mobile/android-ndk-r8b/toolchains/arm-linux-androideabi-4.7.2/prebuilt/linux-x86/bin//arm-linux-androideabi-g++
CFLAGS = --sysroot=/home/mat/docs/dev/mobile/android-ndk-r8b/platforms/android-8/arch-arm/ -Wall -O3 -funroll-loops -mhard-float -mfpu=vfp -mfloat-abi=softfp -fomit-frame-pointer -DANDROID=1 -I/home/mat/docs/dev/mobile/android-ndk-r8b/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include/ -I/home/mat/docs/dev/mobile/android-ndk-r8b/sources/cxx-stl/gnu-libstdc++/4.6/include/ -I/home/mat/docs/src/android/installdir/include -I.. -I$(BOINCDIR) -I$(BOINCDIR)/lib -I$(BOINCDIR)/api $(INFORMIX_CFLAGS) $(MYSQL_CFLAGS) 
CXXFLAGS = $(CFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $*.o -c $<


all: dependencies app_config.o schema_master_client.o sqlrow_client.o schema_master.o sqlrow.o sqlifx.o sqlblob.o sqlint8.o xml_util.o

schema_master.cpp: schema_master.sql find_references.awk schema_to_class.awk
	chmod +x schema_to_class
	./schema_to_class schema_master.sql

schema_master.h: schema_master.sql find_references.awk schema_to_class.awk
	chmod +x schema_to_class
	./schema_to_class schema_master.sql


schema_master_client.o: schema_master.cpp schema_master.h db_table.h Makefile
	$(CXX) $(CXXFLAGS) -DCLIENT -c -o schema_master_client.o schema_master.cpp

sqlrow_client.o: sqlrow.cpp sqlrow.h Makefile
	$(CXX) $(CXXFLAGS) -DCLIENT -c -o sqlrow_client.o sqlrow.cpp

schema_master.o: schema_master.cpp schema_master.h db_table.h 

sqlifx.cpp: sqlifx.ec
	$(INFORMIXDIR)/bin/esql -e $<
	mv $*.c $*.cpp

sqlifx.o: sqlifx.cpp

xml_util.o: xml_util.cpp

app_config.o: app_config.cpp

.ec.cpp:
	$(INFORMIXDIR)/bin/esql -e $<
	mv $*.c $*.cpp

Makefile: Makefile.in ../config.status ../configure ../configure.ac
	(cd ..; $(MAKE) config.status)

dependencies: schema_master.cpp *.cpp *.h 
	$(CXX) $(CXXFLAGS) -M *.cpp > dependencies
        


include dependencies
