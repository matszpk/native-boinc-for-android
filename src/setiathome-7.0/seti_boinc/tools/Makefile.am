## $Id: Makefile.am,v 1.1.2.2 2006/01/04 00:47:33 korpela Exp $

include $(top_srcdir)/Makefile.incl

LDFLAGS=
AM_LDFLAGS=

BOINC_LIBS = -L$(BOINCDIR)/api -lboinc_api -L$(BOINCDIR)/lib -lboinc 

CLIENT_C_FLAGS = $(CFLAGS) \
                $(DEFS) \
		-DTEXT_UI -DNDEBUG -DCLIENT \
		-include $(top_builddir)/config.h \
		-I$(top_srcdir)/db  \
		-I$(top_srcdir)/client \
		$(BOINC_CFLAGS)  \
		$(PTHREAD_CFLAGS) 

CLIENT_LD_FLAGS = $(PTHREAD_CFLAGS) $(LDFLAGS) $(PTHREAD_LIBS) $(BOINC_LIBS) $(APP_LIBS)

noinst_PROGRAMS = fakedata workunit_resample

fakedata_SOURCES = \
    fakedata.cpp \
    ../client/seti_header.cpp \
    ../client/timecvt.cpp \
    ../db/schema_master.cpp \
    ../db/sqlrow.cpp \
    ../db/sqlblob.cpp \
    ../db/xml_util.cpp 

fakedata_CFLAGS = $(CLIENT_C_FLAGS)
fakedata_CXXFLAGS = $(CLIENT_C_FLAGS)
fakedata_LDFLAGS = $(CLIENT_LD_FLAGS) 

workunit_resample_SOURCES = \
    workunit_resample.cpp \
    ../client/seti_header.cpp \
    ../client/timecvt.cpp \
    ../client/s_util.cpp \
    ../db/schema_master.cpp \
    ../db/sqlrow.cpp \
    ../db/sqlblob.cpp \
    ../db/xml_util.cpp 

workunit_resample_CFLAGS = $(CLIENT_C_FLAGS)
workunit_resample_CXXFLAGS = $(CLIENT_C_FLAGS)
workunit_resample_LDFLAGS = $(CLIENT_LD_FLAGS) 
workunit_resample_LDADD = -lfftw3f

