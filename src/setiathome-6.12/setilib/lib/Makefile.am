
ROOT 		= 	$(top_srcdir)
INCLUDEPATH 	= 	$(ROOT)/include
BINPATH  	= 	$(ROOT)/bin
LIBPATH		=	$(ROOT)/lib
SRCPATH		=	$(ROOT)/src
BOINCDIR    = @BOINCDIR@
SETIDIR     = @SETI_BOINC_DIR@
INFORMIX_CFLAGS=@INFORMIX_CFLAGS@
HEALPIX = @HEALPIX@
HEALPIX_INC     = -I$(HEALPIX)/include
HEALPIX_LIBS    = -L$(HEALPIX)/lib -lchealpix

AM_CXXFLAGS = -pg -include ../seti_config.h -I.. -I../AO_code/aocoord -I$(INCLUDEPATH) -I$(SETIDIR)/db -I$(SETIDIR) -I$(BOINCDIR) -I$(BOINCDIR)/lib $(CFLAGS) $(INFORMIX_CFLAGS) $(MYSQL_CFLAGS) $(HEALPIX_INC) $(HEALPIX_LIBS)

AM_CFLAGS = -pg -include ../seti_config.h -I.. -I../AO_code/aocoord -I$(INCLUDEPATH) -I$(SETIDIR)/db -I$(SETIDIR) -I$(BOINCDIR) -I$(BOINCDIR)/lib $(CFLAGS) $(INFORMIX_CFLAGS) $(MYSQL_CFLAGS) $(HEALPIX_INC) $(HEALPIX_LIBS)


lib_LIBRARIES=libseti.a

libseti_a_SOURCES = $(ROOT)/src/seti_doppler.cpp \
                    $(ROOT)/src/seti_cfg.cpp \
		    $(ROOT)/src/seti_time.cpp \
		    $(ROOT)/src/seti_coord.cpp \
		    $(ROOT)/src/seti_healpix.cpp \
		    $(ROOT)/src/seti_tel.cpp \
		    $(ROOT)/src/seti_dr2utils.cpp \
		    $(ROOT)/src/seti_aoutils.cpp \
		    $(ROOT)/src/seti_dr2filter.cpp \
		    $(ROOT)/src/seti_synthetic_data.cpp \
		    $(ROOT)/src/seti_signal.cpp \
		    $(ROOT)/src/seti_rfi.cpp \
		    $(ROOT)/src/seti_rfi_null.cpp \
		    $(ROOT)/src/seti_rfi_zone.cpp \
		    $(ROOT)/src/seti_rfi_staff.cpp \
	        $(ROOT)/src/mtrand.cpp \
		    $(ROOT)/AO_code/aocoord/M3D_Transpose.c \
		    $(ROOT)/AO_code/aocoord/MM3D_Mult.c \
		    $(ROOT)/AO_code/aocoord/MV3D_Mult.c \
		    $(ROOT)/AO_code/aocoord/V3D_Normalize.c \
		    $(ROOT)/AO_code/aocoord/VV3D_Sub.c \
		    $(ROOT)/AO_code/aocoord/aberAnnual_V.c \
		    $(ROOT)/AO_code/aocoord/anglesToVec3.c \
		    $(ROOT)/AO_code/aocoord/azElToHa_V.c \
		    $(ROOT)/AO_code/aocoord/azzaToRaDec.c \
		    $(ROOT)/AO_code/aocoord/azzaToRaDecInit.c \
		    $(ROOT)/AO_code/aocoord/dms3_rad.c \
		    $(ROOT)/AO_code/aocoord/dmToDayNo.c \
		    $(ROOT)/AO_code/aocoord/fmtDms.c \
		    $(ROOT)/AO_code/aocoord/fmtHmsD.c \
		    $(ROOT)/AO_code/aocoord/fmtRdToDms.c \
		    $(ROOT)/AO_code/aocoord/fmtRdToHmsD.c \
		    $(ROOT)/AO_code/aocoord/fmtSMToHmsD.c \
		    $(ROOT)/AO_code/aocoord/gregToMjd.c \
		    $(ROOT)/AO_code/aocoord/gregToMjdDno.c \
		    $(ROOT)/AO_code/aocoord/haToAzEl_V.c \
		    $(ROOT)/AO_code/aocoord/haToRaDec_V.c \
		    $(ROOT)/AO_code/aocoord/hms3_rad.c \
		    $(ROOT)/AO_code/aocoord/isLeapYear.c \
		    $(ROOT)/AO_code/aocoord/meanEqToEcl_A.c \
		    $(ROOT)/AO_code/aocoord/mjdToJulDate.c \
		    $(ROOT)/AO_code/aocoord/nutation_M.c \
		    $(ROOT)/AO_code/aocoord/obsPosInp.c \
		    $(ROOT)/AO_code/aocoord/precJ2ToDate_M.c \
		    $(ROOT)/AO_code/aocoord/precNut.c \
		    $(ROOT)/AO_code/aocoord/precNutInit.c \
		    $(ROOT)/AO_code/aocoord/rad_dms3.c \
		    $(ROOT)/AO_code/aocoord/rad_hms3.c \
		    $(ROOT)/AO_code/aocoord/rotationX_M.c \
		    $(ROOT)/AO_code/aocoord/rotationY_M.c \
		    $(ROOT)/AO_code/aocoord/rotationZ_M.c \
		    $(ROOT)/AO_code/aocoord/secMid_hms3.c \
		    $(ROOT)/AO_code/aocoord/setSign.c \
		    $(ROOT)/AO_code/aocoord/truncDtoI.c \
		    $(ROOT)/AO_code/aocoord/ut1ToLmst.c \
		    $(ROOT)/AO_code/aocoord/utcInfoInp.c \
		    $(ROOT)/AO_code/aocoord/utcToUt1.c \
		    $(ROOT)/AO_code/aocoord/vec3ToAngles.c 


