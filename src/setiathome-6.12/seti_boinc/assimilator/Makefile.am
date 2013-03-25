## $Id: Makefile.am,v 1.2.2.6 2007/08/09 23:37:34 jeffc Exp $
include $(top_srcdir)/Makefile.incl

AM_CFLAGS = @CFLAGS@ @DEFS@ -DTEXT_UI -DNDEBUG @PTHREAD_CFLAGS@ 
AM_CXXFLAGS = $(AM_CFLAGS) 

AM_LDFLAGS += -Xlinker -R -Xlinker $(INFORMIXDIR)/lib:$(INFORMIXDIR)/lib/esql:$(LD_LIBRARY_PATH) @LDFLAGS@ -static

CLIBS = @LIBS@ 

BOINC_INC	= -I$(BOINCDIR) -I$(BOINCDIR)/api -I$(BOINCDIR)/lib -I$(BOINCDIR)/sched -I$(BOINCDIR)/db
BOINC_LIBS 	=  -L$(BOINCDIR)/sched -lsched @MYSQL_LIBS@ @PTHREAD_LIBS@ -L$(BOINCDIR)/lib -lboinc -L$(SSLDIR) -lcrypto -lssl
HEALPIX_INC     = -I$(HEALPIX)/include 
HEALPIX_LIBS    = -L$(HEALPIX)/lib -lchealpix -lhealpix_cxx -lcxxsupport

#bin_PROGRAMS = sah_assimilator
noinst_PROGRAMS = sah_assimilator

sah_assimilator_SOURCES = \
    $(BOINCDIR)/sched/assimilator.cpp \
    $(BOINCDIR)/sched/validate_util.cpp \
    sah_assimilate_handler.cpp \
    ../client/timecvt.cpp \
    ../db/schema_master.cpp \
    ../db/sqlifx.cpp \
    ../db/sqlrow.cpp \
    ../db/sqlblob.cpp \
    ../db/xml_util.cpp \
    ../db/app_config.cpp

sah_assimilator_CFLAGS = $(AM_CFLAGS) $(BOINC_INC) $(HEALPIX_INC) $(SETILIB_CFLAGS) $(MYSQL_CFLAGS) $(INFORMIX_CFLAGS) -I.. -I../db $(PTHREAD_CFLAGS) -I../client -I../validate
sah_assimilator_CXXFLAGS = $(AM_CXXFLAGS) $(BOINC_INC) $(SETILIB_CFLAGS) $(HEALPIX_INC) $(MYSQL_CFLAGS) $(INFORMIX_CFLAGS) -I.. -I../db $(PTHREAD_CFLAGS) -I../client -I../validate
sah_assimilator_LDADD =  $(PTHREAD_CFLAGS) $(BOINC_LIBS) $(SETILIB_LIBS) $(MYSQL_LIBS) $(INFORMIX_LIBS) $(CLIBS) $(HEALPIX_LIBS)

