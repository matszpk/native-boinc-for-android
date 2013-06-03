## $Id: Makefile.am,v 1.7.2.21 2007/08/01 00:15:31 korpela Exp $

include $(top_srcdir)/Makefile.incl


BOINC_LIBS = -L$(BOINCDIR)/api -L$(BOINCDIR)/api/.libs -lboinc_api -L$(BOINCDIR)/lib -L$(BOINCDIR)/lib/.libs -lboinc 

CLIENT_C_FLAGS = $(APP_CFLAGS) $(CFLAGS) \
                $(DEFS) \
		-DTEXT_UI -DNDEBUG -DCLIENT \
		-I$(top_srcdir)/db  \
		$(BOINC_CFLAGS)  \
		$(PTHREAD_CFLAGS)  \
		$(ASMLIB_CFLAGS)
CLIENT_LD_FLAGS = $(PTHREAD_CFLAGS) $(LDFLAGS) $(APP_LDFLAGS)
CLIENT_LD_ADD = $(PTHREAD_LIBS) $(BOINC_LIBS) $(APP_LIBS)

	

if ENABLE_GUI
     CLIENT_BIN = $(CLIENT_PROG) $(CLIENT_NATIVE) $(DEBUG_NATIVE) $(DEBUG_PROG) 
     GUI_BIN = seti_graphics
     GUI_C_FLAGS = $(CLIENT_C_FLAGS) $(GRAPHICS_CFLAGS)
     GUI_LD_ADD = $(GRAPHICS_LIBS_RAW)
     CLIENT_LD_FLAGS += $(LDSTATIC)
     CLIENT_LD_ADD += -L$(BOINCDIR)/api -L$(BOINCDIR)/lib $(BOINC_LIBS) $(AM_LIBS)
     SAH_GRX_SOURCES = sah_gfx_main.cpp 
else
#no graphics
     SAH_GRX_SOURCES =
     CLIENT_LD_ADD += $(AM_LIBS)
     CLIENT_BIN = $(CLIENT_PROG) $(DEBUG_PROG) 
endif

if X86_64
  CLIENT_C_FLAGS+=-msse2 -mfpmath=sse -DUSE_SSE -DUSE_SSE2 
endif

all: client-bin

client-bin: $(CLIENT_BIN)

$(DEBUG_PROG): seti_boinc
	@RM@ -f $(DEBUG_PROG)
	@LN@ seti_boinc $(DEBUG_PROG)

$(CLIENT_PROG): seti_boinc
	@CP@ seti_boinc $(CLIENT_PROG)
	@STRIP@ $(CLIENT_PROG)



noinst_PROGRAMS = seti_boinc 

if ENABLE_TESTS
noinst_PROGRAMS += hires_timer_test

hires_timer_test_SOURCES= vector/hires_timer.cpp
hires_timer_test_CXXFLAGS=-DTEST_TIMER $(CXXFLAGS) $(CLIENT_C_FLAGS)
hires_timer_test_LDFLAGS=$(CLIENT_LD_FLAGS)
hires_timer_test_LDADD=$(CLIENT_LD_ADD)
endif

if ENABLE_ARMOPT
ARMOPT_SOURCES = \
    vector/neon_ChirpData.S \
    vector/vfp_ChirpData.S \
    vector/neon_FoldSubs.S \
    vector/vfp_FoldSubs.S \
    vector/neon_GetPowerSpectrum.S \
    vector/vfp_GetPowerSpectrum.S \
    setupfpu.S 
else
ARMOPT_SOURCES = 
endif

if ENABLE_PFFFT
PFFFT_SOURCES = pffft/pffft.c
else
PFFFT_SOURCES =
endif

seti_boinc_SOURCES = \
    main.cpp \
    vector/analyzeFuncs_vector.cpp \
    vector/analyzeFuncs_fpu.cpp \
    vector/analyzeFuncs_sse.cpp \
    vector/analyzeFuncs_sse2.cpp \
    vector/analyzeFuncs_sse3.cpp \
    vector/analyzeFuncs_avx.cpp \
    vector/analyzeFuncs_x86_64.cpp \
    vector/analyzeFuncs_altivec.cpp \
    vector/x86_float4.cpp \
    vector/hires_timer.cpp \
    $(ARMOPT_SOURCES) \
    $(PFFFT_SOURCES) \
    analyzeFuncs.cpp \
    analyzeReport.cpp \
    analyzePoT.cpp \
    pulsefind.cpp \
    gaussfit.cpp \
    lcgamm.cpp \
    malloc_a.cpp \
    seti.cpp \
    seti_header.cpp \
    timecvt.cpp \
    s_util.cpp \
    sah_version.cpp \
    worker.cpp \
    chirpfft.cpp \
    spike.cpp \
    autocorr.cpp \
    progress.cpp \
    fft8g.cpp \
    gdata.cpp \
    ../db/schema_master.cpp \
    ../db/sqlrow.cpp \
    ../db/sqlblob.cpp \
    ../db/xml_util.cpp \
    $(SAH_GRX_SOURCES)

seti_boinc_CFLAGS = $(CLIENT_C_FLAGS)
seti_boinc_CXXFLAGS = $(CLIENT_C_FLAGS)
seti_boinc_LDFLAGS = $(CLIENT_LD_FLAGS) 
seti_boinc_LDADD = $(CLIENT_LD_ADD) 
seti_boinc_LINK = $(CXX) $(seti_boinc_CXXFLAGS) $(seti_boinc_LDFLAGS) -o $@ 

if I386 
# allow use of sse instructions on i[3456]86
seti_boinc-analyzeFuncs_sse3.o:  vector/analyzeFuncs_sse3.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE -DUSE_SSE2 \
	-DUSE_SSE3 -D__SSE__ -D__SSE2__ -D__SSE3__ \
        -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -msse3 \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-analyzeFuncs_sse2.o:  vector/analyzeFuncs_sse2.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE -DUSE_SSE2 \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -msse2 \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-analyzeFuncs_sse.o:  vector/analyzeFuncs_sse.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -msse \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-analyzeFuncs_x86_64.o: vector/analyzeFuncs_x86_64.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -mfpmath=sse -msse2 \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-x86_float4.o:  vector/x86_float4.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -msse \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
if AVX
seti_boinc-analyzeFuncs_avx.o:  vector/analyzeFuncs_avx.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE -DUSE_SSE2 \
	-DUSE_SSE3 -DUSE_AVX -D__SSE__ -D__SSE2__ -D__SSE3__ \
        -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -mavx \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-analyzeFuncs_vector.o:  vector/analyzeFuncs_vector.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE  -DUSE_SSE2 \
	-DUSE_SSE3 -DUSE_3DNOW -DUSE_AVX \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
else
seti_boinc-analyzeFuncs_vector.o:  vector/analyzeFuncs_vector.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE  -DUSE_SSE2 \
	-DUSE_SSE3 -DUSE_3DNOW \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
endif
endif

if X86_64 
# allow use of sse instructions on i[3456]86
seti_boinc-analyzeFuncs_sse3.o:  vector/analyzeFuncs_sse3.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE3 \
        -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -msse3 \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
if AVX
seti_boinc-analyzeFuncs_avx.o:  vector/analyzeFuncs_avx.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE3 -DUSE_AVX \
        -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -mavx \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-analyzeFuncs_vector.o:  vector/analyzeFuncs_vector.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE3 -DUSE_3DNOW -DUSE_AVX \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
else
seti_boinc-analyzeFuncs_vector.o:  vector/analyzeFuncs_vector.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_SSE3 -DUSE_3DNOW \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
endif
endif

if PPC
# allow use of altivec instructions on PPC
seti_boinc-analyzeFuncs_altivec.o:  vector/analyzeFuncs_altivec.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_ALTIVEC \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -faltivec \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
seti_boinc-analyzeFuncs_vector.o:  vector/analyzeFuncs_vector.cpp
	if $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(seti_boinc_CXXFLAGS) $(CXXFLAGS) -DUSE_ALTIVEC \
	-MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" \
	-c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; \
	else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
endif


if LINUX
# too much optimization on main.cpp breaks the client for some reason
seti_boinc-main.o: main.cpp
	if $(CXX) --include ../sah_config.h $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(DEFS) -DTEXT_UI -DNDEBUG -DCLIENT $(CLIENT_C_FLAGS) -I$(top_srcdir)/db $(BOINC_CFLAGS) $(PTHREAD_CFLAGS) -O2 -Wall -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o $@ $<; then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
endif


if ENABLE_GUI
noinst_PROGRAMS += seti_graphics

seti_graphics_SOURCES = \
    timecvt.cpp \
    sah_gfx.cpp \
    sah_gfx_base.cpp \
    graphics_main.cpp \
    sah_version.cpp 

seti_graphics_CFLAGS = $(GUI_C_FLAGS)
seti_graphics_CXXFLAGS = $(GUI_C_FLAGS)
seti_graphics_LDFLAGS = $(GUI_LD_FLAGS)



seti_graphics_LDADD = $(GUI_LD_ADD) $(BOINC_LIBS)

$(DEBUG_NATIVE): $(GUI_BUILD)
	@RM@ -f $(DEBUG_NATIVE)
	@LN@ sah_graphics $(DEBUG_NATIVE)

$(CLIENT_NATIVE): $(DEBUG_NATIVE)
	@RM@ -f $(CLIENT_NATIVE)
	@CP@ $(DEBUG_NATIVE) $(CLIENT_NATIVE)

endif

