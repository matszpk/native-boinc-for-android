####
#
# handmade milkyway_separation makefile (for android)
#
#####

all: milkyway_separation

clean:
	rm -f *.o milkyway_separation

BOINC_INCDIR = -I$(BOINCROOTDIR)/include/boinc
BOINC_LIBS = $(BOINCROOTDIR)/lib/libboinc_api.a $(BOINCROOTDIR)/lib/libboinc.a

LUASRCS = ../lua/src/lapi.c \
	../lua/src/lauxlib.c \
	../lua/src/lbaselib.c \
	../lua/src/lcode.c \
	../lua/src/ldblib.c \
	../lua/src/ldebug.c \
	../lua/src/ldo.c \
	../lua/src/ldump.c \
	../lua/src/lfunc.c \
	../lua/src/lgc.c \
	../lua/src/linit.c \
	../lua/src/liolib.c \
	../lua/src/llex.c \
	../lua/src/lmathlib.c \
	../lua/src/lmem.c \
	../lua/src/loadlib_rel.c \
	../lua/src/lobject.c \
	../lua/src/lopcodes.c \
	../lua/src/loslib.c \
	../lua/src/lparser.c \
	../lua/src/lstate.c \
	../lua/src/lstring.c \
	../lua/src/lstrlib.c \
	../lua/src/ltable.c \
	../lua/src/ltablib.c \
	../lua/src/ltm.c \
	../lua/src/lundump.c \
	../lua/src/lvm.c \
	../lua/src/lzio.c \
	../lua/src/print.c

POPTSRCS = ../popt/src/lookup3.c \
	../popt/src/popt.c \
	../popt/src/poptconfig.c \
	../popt/src/popthelp.c \
	../popt/src/poptint.c \
	../popt/src/poptparse.c

MWSRCS = ../milkyway/src/milkyway_cpp_util.cc \
	../milkyway/src/milkyway_lua_dsfmt.c \
	../milkyway/src/milkyway_lua_marshal.c \
	../milkyway/src/milkyway_lua_math.c \
	../milkyway/src/milkyway_lua_types.c \
	../milkyway/src/milkyway_lua_util.c \
	../milkyway/src/milkyway_lua_vector.c \
	../milkyway/src/milkyway_show.c \
	../milkyway/src/milkyway_util.c \
	../milkyway/src/milkyway_util_arm.S \
	../milkyway/src/mw_asprintf.c \
	../milkyway/src/mw_boinc_util.c

MWARMSRCS = ../milkyway/src/arm_math/arm_exp.c \
	../milkyway/src/arm_math/arm_log.c \
	../milkyway/src/arm_math/fp2intfp-arm.S \
	../milkyway/src/arm_math/ifpexp.S \
	../milkyway/src/arm_math/ifplog.S
	
SEPSRCS = ../separation/src/calculated_constants.c \
	../separation/src/coordinates.c \
	../separation/src/evaluation.c \
	../separation/src/evaluation_state.c \
	../separation/src/gauss_legendre.c \
	../separation/src/integrals.c \
	../separation/src/io_util.c \
	../separation/src/likelihood.c \
	../separation/src/parameters.c \
	../separation/src/probabilities.c \
	../separation/src/prob_arm_novfp.S \
	../separation/src/prob_arm_vfp.S \
	../separation/src/prob_arm_vfpv3.S \
	../separation/src/r_points.c \
	../separation/src/separation_lua.c \
	../separation/src/separation_main.c \
	../separation/src/separation_utils.c \
	../separation/src/star_points.c

DSFMTSRCS = ../dSFMT/dSFMT.c

SRCS = $(SEPSRCS) $(MWSRCS) $(MWARMSRCS) $(LUASRCS) $(POPTSRCS) $(DSFMTSRCS)

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

MWHDRS = ../milkyway/include/milkyway_cl.h \
	../milkyway/include/milkyway_cpp_util.h \
	../milkyway/include/milkyway_extra.h \
	../milkyway/include/milkyway_lua_dsfmt.h \
	../milkyway/include/milkyway_lua.h \
	../milkyway/include/milkyway_lua_marshal.h \
	../milkyway/include/milkyway_lua_math.h \
	../milkyway/include/milkyway_lua_types.h \
	../milkyway/include/milkyway_lua_util.h \
	../milkyway/include/milkyway_lua_vector.h \
	../milkyway/include/milkyway_math_functions.h \
	../milkyway/include/milkyway_math.h \
	../milkyway/include/milkyway_show.h \
	../milkyway/include/milkyway_util.h \
	../milkyway/include/milkyway_vectors.h \
	../milkyway/include/milkyway_vector_types.h \
	../milkyway/include/mw_asprintf.h \
	../milkyway/include/mw_boinc_util.h \
	../milkyway/include/mw_cl_device.h \
	../milkyway/include/mw_cl.h \
	../milkyway/include/mw_cl_program.h \
	../milkyway/include/mw_cl_setup.h \
	../milkyway/include/mw_cl_show_types.h \
	../milkyway/include/mw_cl_types.h \
	../milkyway/include/mw_cl_util.h \
	../milkyway/include/real.h

MWARMHDRS = ../milkyway/include/arm_math/arm_exp.h \
	../milkyway/include/arm_math/arm_log.h \
	../milkyway/include/arm_math/fp2intfp.h

SEPHDRS = ../separation/include/calculated_constants.h \
	../separation/include/coordinates.h \
	../separation/include/evaluation.h \
	../separation/include/evaluation_state.h \
	../separation/include/gauss_legendre.h \
	../separation/include/integrals.h \
	../separation/include/integrals_likelihood.h \
	../separation/include/io_util.h \
	../separation/include/likelihood.h \
	../separation/include/parameters.h \
	../separation/include/probabilities.h \
	../separation/include/r_points.h \
	../separation/include/run_cl.h \
	../separation/include/separation_binaries.h \
	../separation/include/separation_cal.h \
	../separation/include/separation_cal_kernelgen.h \
	../separation/include/separation_cal_run.h \
	../separation/include/separation_cal_setup.h \
	../separation/include/separation_cal_types.h \
	../separation/include/separation_cl_buffers.h \
	../separation/include/separation_cl_defs.h \
	../separation/include/separation_constants.h \
	../separation/include/separation_graphics.h \
	../separation/include/separation.h \
	../separation/include/separation_kernel_types.h \
	../separation/include/separation_lua.h \
	../separation/include/separation_types.h \
	../separation/include/separation_utils.h \
	../separation/include/setup_cl.h \
	../separation/include/show_cal_types.h \
	../separation/include/star_points.h

CONFHDRS = config/luaconf.h config/milkyway_config.h \
	config/nbody_config.h config/separation_config.h

DSFMTHDRS = ../dSFMT/dSFMT.h

HDRS = $(SEPHDRS) $(MWHDRS) $(MWARMHDRS) $(LUAHDRS) $(POPTHDRS) $(DSFMTHDRS) $(CONFHDRS)

LUAOBJS = lapi.o lauxlib.o lbaselib.o lcode.o \
	ldblib.o ldebug.o ldo.o ldump.o \
	lfunc.o lgc.o linit.o liolib.o \
	llex.o lmathlib.o lmem.o loadlib_rel.o \
	lobject.o lopcodes.o loslib.o \
	lparser.o lstate.o lstring.o lstrlib.o \
	ltable.o ltablib.o ltm.o \
	lundump.o lvm.o lzio.o print.o

POPTOBJS = lookup3.o popt.o poptconfig.o popthelp.o poptint.o poptparse.o

DSFMTOBJS = dSFMT.o

MWOBJS = milkyway_cpp_util.o milkyway_lua_marshal.o \
	milkyway_lua_dsfmt.o milkyway_lua_math.o \
	milkyway_lua_types.o milkyway_lua_util.o \
	milkyway_lua_vector.o milkyway_show.o \
	milkyway_util.o milkyway_util_arm.o mw_asprintf.o \
	mw_boinc_util.o

MWARMOBJS = arm_exp.o arm_log.o fp2intfp-arm.o ifpexp.o ifplog.o

SEPOBJS = calculated_constants.o coordinates.o \
	evaluation.o evaluation_state.o \
	gauss_legendre.o integrals.o \
	io_util.o likelihood.o parameters.o \
	probabilities.o prob_arm_novfp.o \
	prob_arm_vfp.o prob_arm_vfpv3.o  \
	r_points.o separation_lua.o separation_main.o \
	separation_utils.o star_points.o

OBJS = $(SEPOBJS) $(MWOBJS) $(MWARMOBJS) $(LUAOBJS) $(POPTOBJS) $(DSFMTOBJS)

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
DEFS = -DSKIP_VFP_SAVE=1 -DRDEBUG=1 -DTEST_NOVFP=1 -DWRITE_PROGRESS=1 -DMILKYWAY_LUA_COMPILATION=1 \
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
