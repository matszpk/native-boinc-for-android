####
#
# handmade milkyway_separation makefile (for android)
#
#####

all: milkyway_separation

clean:
	rm -f *.o milkyway_separation

BOINC_INCDIR = -I/data/local/tmp/include/boinc
BOINC_LIBS = /data/local/tmp/lib/libboinc_api.a /data/local/tmp/lib/libboinc.a

######################
# sources list
LUASRCS = ../lua/src/lapi.c \
	../lua/src/lcode.c \
	../lua/src/ldebug.c \
	../lua/src/ldo.c \
	../lua/src/ldump.c  \
	../lua/src/lfunc.c \
	../lua/src/lgc.c \
	../lua/src/llex.c \
	../lua/src/lmem.c \
	../lua/src/lobject.c \
	../lua/src/lopcodes.c \
	../lua/src/lparser.c \
	../lua/src/lstate.c \
	../lua/src/lstring.c \
	../lua/src/ltable.c \
	../lua/src/ltm.c \
	../lua/src/lundump.c \
	../lua/src/lvm.c \
	../lua/src/lzio.c \
	../lua/src/lauxlib.c \
	../lua/src/lbaselib.c \
	../lua/src/ldblib.c \
	../lua/src/liolib.c \
	../lua/src/lmathlib.c \
	../lua/src/loslib.c \
	../lua/src/ltablib.c \
	../lua/src/lstrlib.c \
	../lua/src/linit.c \
	../lua/src/loadlib_rel.c

POPTSRCS = ../popt/src/popt.c \
	../popt/src/poptconfig.c \
	../popt/src/popthelp.c \
	../popt/src/poptint.c \
	../popt/src/poptparse.c

MWSRCS = ../milkyway/src/milkyway_util.c \
	../milkyway/src/milkyway_alloc.c \
	../milkyway/src/milkyway_rename.c \
	../milkyway/src/milkyway_boinc_util.cc \
	../milkyway/src/milkyway_show.c \
	../milkyway/src/milkyway_cpuid.c \
	../milkyway/src/milkyway_timing.c \
	../milkyway/src/milkyway_lua_marshal.c \
	../milkyway/src/milkyway_lua_util.c \
	../milkyway/src/milkyway_lua_math.c \
	../milkyway/src/milkyway_lua_vector.c \
	../milkyway/src/milkyway_lua_dsfmt.c \
	../milkyway/src/milkyway_lua_types.c \
	../milkyway/src/boinc/parse_new.cpp \
	../milkyway/src/boinc/str_replace.cpp \
	../milkyway/src/milkyway_util_arm.S \
	../milkyway/src/arm_math/arm_exp.c \
	../milkyway/src/arm_math/arm_log.c \
	../milkyway/src/arm_math/fp2intfp-arm.S \
	../milkyway/src/arm_math/ifpexp.S \
	../milkyway/src/arm_math/ifplog.S

SEPSRCS = ../separation/src/separation_main.c \
	../separation/src/evaluation_state.c \
	../separation/src/io_util.c \
	../separation/src/gauss_legendre.c \
	../separation/src/evaluation.c \
	../separation/src/probabilities.c \
	../separation/src/probabilities_dispatch.c \
	../separation/src/parameters.c \
	../separation/src/star_points.c \
	../separation/src/likelihood.c \
	../separation/src/coordinates.c \
	../separation/src/integrals.c \
	../separation/src/calculated_constants.c \
	../separation/src/separation_utils.c \
	../separation/src/r_points.c \
	../separation/src/separation_lua.c \
	../separation/src/prob_arm_novfp.S \
	../separation/src/prob_arm_vfp.S \
	../separation/src/prob_arm_vfpv3.S

DSFMTSRCS = ../dSFMT/dSFMT.c

SRCS = $(SEPSRCS) $(MWSRCS) $(LUASRCS) $(POPTSRCS) $(DSFMTSRCS)

######################
# headers list

LUAHDRS = ../lua/include/lapi.h \
	../lua/include/lauxlib.h \
	../lua/include/lcode.h \
	../lua/include/ldebug.h \
	../lua/include/ldo.h \
	../lua/include/lfunc.h \
	../lua/include/lgc.h \
	../lua/include/llex.h \
	../lua/include/llimits.h \
	../lua/include/lmem.h \
	../lua/include/lobject.h \
	../lua/include/lopcodes.h \
	../lua/include/lparser.h \
	../lua/include/lstate.h \
	../lua/include/lstring.h \
	../lua/include/ltable.h \
	../lua/include/ltm.h \
	../lua/include/lua.h \
	../lua/include/lualib.h \
	../lua/include/lundump.h \
	../lua/include/lvm.h \
	../lua/include/lzio.h

POPTHDRS = ../popt/include/popt.h \
	../popt/include/poptint.h \
	../popt/include/system.h

MWHDRS = ../milkyway/include/milkyway_extra.h \
	../milkyway/include/milkyway_math.h \
	../milkyway/include/milkyway_math_double.h \
	../milkyway/include/milkyway_math_float.h \
	../milkyway/include/milkyway_math_supplemental.h \
	../milkyway/include/milkyway_util.h \
	../milkyway/include/milkyway_alloc.h \
	../milkyway/include/milkyway_boinc_util.h \
	../milkyway/include/milkyway_rename.h \
	../milkyway/include/milkyway_vectors.h \
	../milkyway/include/milkyway_show.h \
	../milkyway/include/milkyway_cpuid.h \
	../milkyway/include/milkyway_timing.h \
	../milkyway/include/milkyway_asprintf.h \
	../milkyway/include/milkyway_lua.h \
	../milkyway/include/milkyway_lua_marshal.h \
	../milkyway/include/milkyway_lua_util.h \
	../milkyway/include/milkyway_lua_math.h \
	../milkyway/include/milkyway_lua_vector.h \
	../milkyway/include/milkyway_lua_dsfmt.h \
	../milkyway/include/milkyway_lua_types.h \
	../milkyway/include/boinc/parse_new.h \
	../milkyway/include/boinc/str_replace.h \
	../milkyway/include/arm_math/arm_exp.h \
	../milkyway/include/arm_math/arm_log.h \
	../milkyway/include/arm_math/fp2intfp.h

SEPHDRS = ../separation/include/calculated_constants.h \
	../separation/include/separation_types.h \
	../separation/include/coordinates.h \
	../separation/include/separation.h \
	../separation/include/evaluation.h \
	../separation/include/probabilities_dispatch.h \
	../separation/include/probabilities.h \
	../separation/include/io_util.h \
	../separation/include/star_points.h \
	../separation/include/evaluation_state.h \
	../separation/include/likelihood.h \
	../separation/include/gauss_legendre.h \
	../separation/include/parameters.h \
	../separation/include/integrals.h \
	../separation/include/r_points.h \
	../separation/include/separation_utils.h \
	../separation/include/separation_constants.h \
	../separation/include/separation_lua.h \
	../separation/include/probabilities.h \
	../separation/include/probabilities_intrin.h

CONFHDRS = config/luaconf.h config/milkyway_config.h \
	config/nbody_config.h config/separation_config.h

DSFMTHDRS = ../dSFMT/dSFMT.h

HDRS = $(SEPHDRS) $(MWHDRS) $(MWARMHDRS) $(LUAHDRS) $(POPTHDRS) $(DSFMTHDRS) $(CONFHDRS)

#########################
# separation objects

LUAOBJS = lapi.o \
	lcode.o \
	ldebug.o \
	ldo.o \
	ldump.o  \
	lfunc.o \
	lgc.o \
	llex.o \
	lmem.o \
	lobject.o \
	lopcodes.o \
	lparser.o \
	lstate.o \
	lstring.o \
	ltable.o \
	ltm.o \
	lundump.o \
	lvm.o \
	lzio.o \
	lauxlib.o \
	lbaselib.o \
	ldblib.o \
	liolib.o \
	lmathlib.o \
	loslib.o \
	ltablib.o \
	lstrlib.o \
	linit.o \
	loadlib_rel.o

POPTOBJS = popt.o \
	poptconfig.o \
	popthelp.o \
	poptint.o \
	poptparse.o

MWOBJS = milkyway_util.o \
	milkyway_alloc.o \
	milkyway_rename.o \
	milkyway_boinc_util.o \
	milkyway_show.o \
	milkyway_cpuid.o \
	milkyway_timing.o \
	milkyway_lua_marshal.o \
	milkyway_lua_util.o \
	milkyway_lua_math.o \
	milkyway_lua_vector.o \
	milkyway_lua_dsfmt.o \
	milkyway_lua_types.o \
	parse_new.o \
	str_replace.o \
	milkyway_util_arm.o \
	arm_exp.o \
	arm_log.o \
	fp2intfp-arm.o \
	ifpexp.o \
	ifplog.o

SEPOBJS = separation_main.o \
	evaluation_state.o \
	io_util.o \
	gauss_legendre.o \
	evaluation.o \
	probabilities.o \
	probabilities_dispatch.o \
	parameters.o \
	star_points.o \
	likelihood.o \
	coordinates.o \
	integrals.o \
	calculated_constants.o \
	separation_utils.o \
	r_points.o \
	separation_lua.o \
	prob_arm_novfp.o \
	prob_arm_vfp.o \
	prob_arm_vfpv3.o

DSFMTOBJS = dSFMT.o
	
OBJS = $(SEPOBJS) $(MWOBJS) $(MWOBJS) $(LUAOBJS) $(POPTOBJS) $(DSFMTOBJS)

LUAINCDIR = -I../lua/include
MWINCDIR = -I../milkyway/include
POPTINCDIR = -I../popt/include
SEPINCDIR = -I../separation/include
DSFMTINCDIR = -I../dSFMT

INCDIRS = -Iconfig $(SEPINCDIR) $(LUAINCDIR) $(MWINCDIR) $(POPTINCDIR) \
	$(DSFMTINCDIR) $(BOINC_INCDIR)

#DEFS = -DWRITE_PROGRESS=1 -DRDEBUG=1 -DTEST_ARMFUNC=1 -DTEST_VFP=1 -DMILKYWAY_LUA_COMPILATION=1 \
#	-DDOUBLEPREC=1 -DDSFMT_MEXP=19937

# If you want to compile with VFP and NEON extensions please remove SKIP_VFP_SAVE macro
DEFS = -DSKIP_VFP_SAVE=1 -DMILKYWAY_LUA_COMPILATION=1 \
	-DDOUBLEPREC=1 -DDSFMT_MEXP=19937
#DEFS = -DSKIP_VFP_SAVE=1 -DRDEBUG=1 -DWRITE_INT_STEPS=1 -DWRITE_PROGRESS=1 -DMILKYWAY_LUA_COMPILATION=1 \
#	-DDOUBLEPREC=1 -DDSFMT_MEXP=19937

DYNLIBS = -lm

###################################
# main rule
######################################
%.o: ../lua/src/%.c
	$(CC) $(CFLAGS) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../popt/src/%.c
	$(CC) $(CFLAGS) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../dSFMT/%.c
	$(CC) $(CFLAGS) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../milkyway/src/%.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../milkyway/src/boinc/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../milkyway/src/%.c
	$(CC) $(CFLAGS) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../milkyway/src/arm_math/%.c
	$(CC) $(CFLAGS) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../milkyway/src/%.S
	$(CC) $(CFLAGS) $(DEFS) -c -o $@ $<

%.o: ../milkyway/src/arm_math/%.S
	$(CC) $(CFLAGS) $(DEFS) -c -o $@ $<

%.o: ../separation/src/%.c
	$(CC) $(CFLAGS) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -c -o $@ $<

%.o: ../separation/src/%.S
	$(CC) $(CFLAGS) $(DEFS) -c -o $@ $<

milkyway_separation: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DYNLIBS) -o milkyway_separation $^ $(BOINC_LIBS) $(STDCPP)

DEPS = make.deps

make.deps: $(SRCS) $(HDRS)
	$(CC) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -MM $(SRCS) > make.deps

include $(DEPS)
