SETI_CFLAGS = -I ../include -I$(SETI_BOINC_DIR) -I$(SETI_BOINC_DIR)/db \
              -I$(SETI_BOINC_DIR)/client -I$(BOINCDIR) \
	      $(BOINC_CFLAGS) $(INFORMIX_CFLAGS) -DUSE_INFORMIX -DUSE_MYSQL \
              $(MYSQL_CFLAGS) -lstdc++ -std=c++0x
SETILIB_CFLAGS = -I.. -I../include $(PTHREAD_CFLAGS)
SETILIB_LDADD = -L$(BOINCDIR)/lib -lboinc -L../lib -lseti $(APP_LIBS) -lm

HEALPIX_INC     = -I$(HEALPIX)/include
HEALPIX_LIBS    = -L$(HEALPIX)/lib -lchealpix

AM_CFLAGS= $(SETILIB_CFLAGS) $(BOINC_CFLAGS) $(SETI_CFLAGS) $(HEALPIX_INC)
AM_CXXFLAGS=$(AM_CFLAGS) 

LIBS+=$(SETILIB_LDADD) $(INFORMIX_LIBS) $(PTHREAD_LIBS) $(HEALPIX_LIBS)


DB_CODE =            $(SETI_BOINC_DIR)/db/schema_master.cpp \
                     $(SETI_BOINC_DIR)/db/sqlifx.cpp \
		     $(SETI_BOINC_DIR)/db/sqlrow.cpp \
                     $(SETI_BOINC_DIR)/db/sqlblob.cpp \
		     $(SETI_BOINC_DIR)/db/sqlint8.cpp \
                     $(SETI_BOINC_DIR)/db/xml_util.cpp \
		     $(SETI_BOINC_DIR)/db/app_config.cpp 

noinst_PROGRAMS = print_vgc_values compare_zenaz_corrections precess2 precess inputs_to_channels dr2_stub_filter print_dr2_data show_radar_blanker print_dr2_headers format_dr2_data signals_by_id_range_demo signals_by_tape_id_demo 

compare_zenaz_corrections_SOURCES = compare_zenaz_corrections.cpp

precess_SOURCES = precess.cc $(DB_CODE)
precess2_SOURCES = precess2.cc $(DB_CODE)
# where is precess3.cc ?
#precess3_SOURCES = precess3.cc $(DB_CODE)

# demo_seti_time_SOURCES = demo_seti_time.cpp $(DB_CODE)

dr2_stub_filter_SOURCES = dr2_stub_filter.cpp $(DB_CODE)

print_dr2_data_SOURCES = print_dr2_data.cc $(DB_CODE)

# structure_dr2_data_SOURCES = structure_dr2_data.cc $(DB_CODE)

show_radar_blanker_SOURCES = show_radar_blanker.cc $(DB_CODE)

inputs_to_channels_SOURCES = inputs_to_channels.cc $(DB_CODE)

print_dr2_headers_SOURCES = print_dr2_headers.cc $(DB_CODE)

print_vgc_values_SOURCES = print_vgc_values.cc $(DB_CODE)

# print_dr2_dsifs_SOURCES = print_dr2_dsifs.cc $(DB_CODE)

format_dr2_data_SOURCES = format_dr2_data.cc $(DB_CODE)

signals_by_id_range_demo_SOURCES = signals_by_id_range_demo.cpp $(DB_CODE)

signals_by_tape_id_demo_SOURCES = signals_by_tape_id_demo.cpp $(DB_CODE)

# deprecated
#printSignals_SOURCES = printSignals.cc $(DB_CODE)
