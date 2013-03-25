BOINCDIR = /home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38
INFORMIXDIR= 
BOINCLIBS=-L$(BOINCDIR)/lib -lboinc 
SETIHOME=../..
DBLIBS=../xml_util.o ../sqlblob.o ../sqlrow.o ../sqlifx.o  -ldl -lm -lstdc++ 

INCLUDE_DIRS= -I$(SETIHOME) -I$(SETIHOME)/client -I$(SETIHOME)/db \
               \
               \
	       \
              -I$(BOINCDIR) -I$(BOINCDIR)/lib -I$(BOINCDIR)/sched -I$(BOINCDIR)/db -I$(BOINCDIR)/tools

LINKOPTIONS=-Xlinker -R -Xlinker $(LD_LIBRARY_PATH):$(INFORMIXDIR)/lib:$(INFORMIXDIR)/lib/esql

SUFFIXES = .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

CC = /home/mat/docs/dev/mobile/android-ndk-r8b/toolchains/arm-linux-androideabi-4.7.2/prebuilt/linux-x86/bin//arm-linux-androideabi-gcc

CFLAGS = --sysroot=/home/mat/docs/dev/mobile/android-ndk-r8b/platforms/android-8/arch-arm/ -Wall -O3 -funroll-loops -mhard-float -mfpu=vfp -mfloat-abi=softfp -fomit-frame-pointer -DANDROID=1 -I/home/mat/docs/dev/mobile/android-ndk-r8b/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include/ -I/home/mat/docs/dev/mobile/android-ndk-r8b/sources/cxx-stl/gnu-libstdc++/4.6/include/ -I/home/mat/docs/src/android/installdir/include -DHAVE_CONFIG_H $(INCLUDE_DIRS) -DUSE_INFORMIX

CXX = /home/mat/docs/dev/mobile/android-ndk-r8b/toolchains/arm-linux-androideabi-4.7.2/prebuilt/linux-x86/bin//arm-linux-androideabi-g++

CXXFLAGS = $(CFLAGS)

dependencies: *.cpp 
	$(CC) $(CFLAGS) -M *.cpp > dependencies

all: dependencies print_s4_receivers insert_analysis_config insert_splitter_config insert_receiver_config insert_rfi_zones

insert_stars: insert_stars.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_stars insert_stars.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_s4_receivers: insert_s4_receivers.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_s4_receivers insert_s4_receivers.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_receiver_config: insert_receiver_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_receiver_config insert_receiver_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_analysis_config: insert_analysis_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_analysis_config insert_analysis_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_rfi_zones: insert_rfi_zones.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_rfi_zones insert_rfi_zones.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_science_config: insert_science_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_science_config insert_science_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

insert_splitter_config: insert_splitter_config.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o insert_splitter_config insert_splitter_config.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

print_s4_receivers: print_s4_receivers.o  ../schema_master.o ../sqlifx.o ../sqlrow.o
	$(CC) $(LINKOPTIONS) -o print_s4_receivers print_s4_receivers.o ../schema_master.o $(S4LIBS) $(DBLIBS) $(BOINCLIBS)

include dependencies
