# Makefile for gcwsieve. (C) Geoffrey Reynolds, March 2007.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.

MAJOR_VER=1
MINOR_VER=3
PATCH_VER=8

# Set DEBUG=yes to compile with debugging information and internal checks.
#
DEBUG=no

# Set ASSEMBLER=yes to use assembler code where available.
#
ASSEMBLER=no

# Uncomment one of the following ARCH= lines:
#
#ARCH=ppc64-osx	    # Apple G5 with MacOS X
#ARCH=ppc64-linux   # Apple G5 with Linux
#ARCH=i586          # Generic Pentium or compatible
#ARCH=i686          # Pentium 2 or Pentium 3
#ARCH=athlon        # Athlon (32-bit)
#ARCH=pentium4      # Pentium 4 (32-bit)
#ARCH=athlon64      # AMD Athlon 64 (in 64-bit mode)
#ARCH=core2         # Intel Core 2 (in 64-bit mode)
#ARCH=x86           # Runs on any x86, uses SSE2 if available (32-bit).
#ARCH=x86-64        # Runs on any x86-64 in 64-bit mode, AMD/Intel code paths.
#ARCH=x86-64-gcc430 # Same as ARCH=x86-64 but allow for bugs in GCC 4.3.0.
#ARCH=x86-osx       # Intel x86 with Mac OS X
#ARCH=x86-64-osx    # Intel x86_64 with Mac OS X

# To link with the BOINC library, uncomment these three lines:
#
BOINC_CPPFLAGS=-DBOINC -I$(BOINCROOTDIR)/include/boinc
BOINC_LDFLAGS=-L/home/geoff/development/boinc/lib64
BOINC_LIBS= $(BOINCROOTDIR)/lib/libboinc_api.a $(BOINCROOTDIR)/lib/libboinc.a $(STDCPP)

# To link with the BOINC library and make the executable work on systems that
# don't have C++ libraries installed, uncomment these three lines:
#
#BOINC_CPPFLAGS=-DBOINC -I/home/geoff/development/boinc/include -fno-exceptions
#BOINC_LDFLAGS=-L/home/geoff/development/boinc/lib64 -static-libgcc
#BOINC_LIBS=-lboinc_api -lboinc /usr/lib/gcc/x86_64-linux-gnu/4.1.2/libstdc++.a -lpthread

# If ARCH not set above then these defauts will be used:
#
ifeq ($(strip $(ARCH)),)
DEBUG_CFLAGS=-g
OPT_CFLAGS=
CPPFLAGS=-Wall
ASSEMBLER=no
# Uncomment to enable software prefetching
#LOOP_OBJS=loop-genericp.o
#CPPFLAGS+= -DPREFETCH_OPT
endif

ifeq ($(strip $(ARCH)),armv6-android)
CPPFLAGS=-DARM_CPU=1 -DARM_NOVFP=1 -DARM_NEON=1  -DMULTI_PATH=1
OPT_CFLAGS=-O3
DEBUG_CFLAGS=-g
ASSEMBLER=yes
PATH1_FLAGS=-DCODE_PATH=1 -DSWIZZLE=2
PATH2_FLAGS=-DCODE_PATH=2 -DSWIZZLE=6
EXTRA_OBJS=revert-loop.o
MULTI_OBJS=sieve2.o revert-loop2.o
endif

# Apple G5 with OS X
#
ifeq ($(strip $(ARCH)),ppc64-osx)
DEBUG_CFLAGS=-g -O3 -ffast-math -mdynamic-no-pic -mtune=G5 -mcpu=970 -m64
OPT_CFLAGS=-O3 -ffast-math -mdynamic-no-pic -mtune=G5 -mcpu=970 -m64 -fomit-frame-pointer
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mdynamic-no-pic -mtune=G5 -mcpu=970 -m64
LDFLAGS=-m64
PATH1_FLAGS=-DSWIZZLE=2
# Uncomment to enable software prefetching
#LOOP_OBJS=loop-genericp.o
#CPPFLAGS+= -DPREFETCH_OPT
endif

# Apple G5 with Linux
#
ifeq ($(strip $(ARCH)),ppc64-linux)
DEBUG_CFLAGS=-g -O3 -ffast-math -mtune=G5 -mcpu=970 -m64 -maltivec -mabi=altivec
OPT_CFLAGS=-O3 -ffast-math -mtune=G5 -mcpu=970 -m64 -fomit-frame-pointer -maltivec -mabi=altivec
CPPFLAGS=-Wall -Xassembler -mregnames -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mregnames -mtune=G5 -mcpu=970 -m64 -maltivec -mabi=altivec
LDFLAGS=-m64
PATH1_FLAGS=-DSWIZZLE=2
# Uncomment to enable software prefetching
#LOOP_OBJS=loop-genericp.o
#CPPFLAGS+= -DPREFETCH_OPT
endif

# Pentium compatible
#
ifeq ($(strip $(ARCH)),i586)
DEBUG_CFLAGS=-g -march=i586 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=i586 -m32
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=128
ASFLAGS=-m32
LDFLAGS=-m32
endif

# Intel Pentium2 or Pentium3
#
ifeq ($(strip $(ARCH)),i686)
DEBUG_CFLAGS=-g -march=i686 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=i686 -m32
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
ASFLAGS=-m32
LDFLAGS=-m32
endif

# AMD Athlon (32-bit)
#
ifeq ($(strip $(ARCH)),athlon)
DEBUG_CFLAGS=-g -march=athlon -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=athlon -m32
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
ASFLAGS=-m32
LDFLAGS=-m32
endif

# Intel Pentium 4 (32-bit)
#
ifeq ($(strip $(ARCH)),pentium4)
DEBUG_CFLAGS=-g -march=pentium4 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=pentium4 -m32
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-m32
LDFLAGS=-m32
endif

# AMD Athlon 64 (in 64-bit mode)
#
ifeq ($(strip $(ARCH)),athlon64)
DEBUG_CFLAGS=-g -march=k8 -m64
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=k8 -m64
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=64 -DDEFAULT_L2_CACHE_SIZE=512
PATH1_FLAGS=-O3
ASFLAGS=-m64
LDFLAGS=-m64
endif

# Intel Core 2 (in 64-bit mode)
#
ifeq ($(strip $(ARCH)),core2)
DEBUG_CFLAGS=-g -march=k8 -mno-3dnow -msse3 -m64
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -msse3 -m64
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=1024
PATH1_FLAGS=-O3
ASFLAGS=-m64
LDFLAGS=-m64
endif

# Runs on any x86, uses SSE2 if available.
#
ifeq ($(strip $(ARCH)),x86)
DEBUG_CFLAGS=-g -march=i586 -mtune=k8 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=i586 -mtune=k8 -m32
CPPFLAGS=-Wall -DMULTI_PATH -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
ASFLAGS=-m32
LDFLAGS=-m32
PATH1_FLAGS=-DCODE_PATH=1 -O3
PATH2_FLAGS=-DCODE_PATH=2 -O3 -march=pentium4 -mtune=k8
MULTI_OBJS=sieve2.o
endif

# Runs on any x86-64 (in 64-bit mode), seperate code paths for AMD/Intel.
#
ifeq ($(strip $(ARCH)),x86-64)
DEBUG_CFLAGS=-g -march=k8 -mno-3dnow -m64
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -m64
CPPFLAGS=-Wall -DMULTI_PATH -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-m64
LDFLAGS=-m64
PATH1_FLAGS=-DCODE_PATH=1 -O3 -msse3
PATH2_FLAGS=-DCODE_PATH=2 -O3
MULTI_OBJS=sieve2.o
endif

# Same as ARCH=x86-64 but allow for bugs in GCC 4.3.0.
#
ifeq ($(strip $(ARCH)),x86-64-gcc430)
DEBUG_CFLAGS=-g -march=k8 -mno-3dnow -m64
OPT_CFLAGS=-O -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -m64
CPPFLAGS=-Wall -DMULTI_PATH -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-m64
LDFLAGS=-m64
PATH1_FLAGS=-DCODE_PATH=1 -msse3
PATH2_FLAGS=-DCODE_PATH=2
MULTI_OBJS=sieve2.o
endif

# Intel x86 with OS X
#
ifeq ($(strip $(ARCH)),x86-osx)
DEBUG_CFLAGS=-g -O3 -ffast-math -mdynamic-no-pic -march=prescott -arch i386
OPT_CFLAGS=-O3 -ffast-math -mdynamic-no-pic -march=prescott -arch i386 -fomit-frame-pointer -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mdynamic-no-pic -march=prescott -arch i386
LDFLAGS=-arch i386 -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif

# Intel x86_64 with OS X
#
ifeq ($(strip $(ARCH)),x86-64-osx)
DEBUG_CFLAGS=-g -O3 -ffast-math -mdynamic-no-pic -march=nocona -arch x86_64
OPT_CFLAGS=-O3 -ffast-math -march=nocona -arch x86_64 -fomit-frame-pointer -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mdynamic-no-pic -march=nocona -arch x86_64
LDFLAGS=-arch x86_64 -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
endif


# Add ARCH-specific assembler objects and flags.
#
ifeq ($(strip $(ASSEMBLER)),yes)
CPPFLAGS+= -DUSE_ASM
ASM_OBJS=
ifeq ($(strip $(ARCH)),ppc64-osx)
#ASM_OBJS=asm-ppc64.o mulmod-ppc64.o expmod-ppc64.o
CPPFLAGS+= -DNEED_UNDERSCORE -DUSE_INLINE_MULMOD
ASM_OBJS=asm-ppc64.o expmod-ppc64.o
endif
ifeq ($(strip $(ARCH)),armv6-android)
CPPFLAGS+= -DSKIP_VFP_SAVE=1
ASM_OBJS=arithmetic-arm.o btop-arm.o arithmetic-neon.o btop-neon.o
LOOP_OBJS=loop-arm.o loop-neon.o
endif
ifeq ($(strip $(ARCH)),ppc64-linux)
#ASM_OBJS=asm-ppc64.o mulmod-ppc64.o expmod-ppc64.o
CPPFLAGS+= -DUSE_INLINE_MULMOD
ASM_OBJS=asm-ppc64.o expmod-ppc64.o
endif
ifeq ($(strip $(ARCH)),i586)
ASM_OBJS=misc-i386.o mulmod-i386.o powmod-i386.o btop-x86.o
LOOP_OBJS=loop-x86.o
PATH1_FLAGS+= -DSWIZZLE=2
endif
ifeq ($(strip $(ARCH)),i686)
ASM_OBJS=misc-i386.o mulmod-i386.o powmod-i386.o btop-x86.o
LOOP_OBJS=loop-x86.o loop-x86.p.o
PATH1_FLAGS+= -DSWIZZLE=2
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),athlon)
ASM_OBJS=misc-i386.o mulmod-i386.o powmod-i386.o btop-x86.o
LOOP_OBJS=loop-x86.o loop-x86.p.o
PATH1_FLAGS+= -DSWIZZLE=2
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),pentium4)
ASM_OBJS=misc-i386.o mulmod-i386.o powmod-sse2.o btop-x86-sse2.o
LOOP_OBJS=loop-x86-sse2.o loop-x86-sse2.p.o
PATH1_FLAGS+= -DSWIZZLE=4
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),athlon64)
ASM_OBJS=misc-x86-64.o mulmod-x86-64.o powmod-x86-64.o btop-x86-64.o
LOOP_OBJS=loop-a64.o loop-a64.p.o
PATH1_FLAGS+= -DSWIZZLE=6
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),core2)
ASM_OBJS=misc-x86-64.o mulmod-x86-64.o powmod-x86-64.o btop-x86-64.o
LOOP_OBJS=loop-core2.o loop-core2.p.o
PATH1_FLAGS+= -DSWIZZLE=4
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),x86)
ASM_OBJS=misc-i386.o mulmod-i386.o powmod-i386.o powmod-sse2.o btop-x86.o btop-x86-sse2.o
LOOP_OBJS=loop-x86.o loop-x86-sse2.o loop-x86.p.o loop-x86-sse2.p.o
PATH1_FLAGS+= -DSWIZZLE=2
PATH2_FLAGS+= -DSWIZZLE=4
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),x86-64)
ASM_OBJS=misc-x86-64.o mulmod-x86-64.o powmod-x86-64.o btop-x86-64.o
LOOP_OBJS=loop-a64.o loop-a64.p.o loop-core2.o loop-core2.p.o
PATH1_FLAGS+= -DSWIZZLE=4
PATH2_FLAGS+= -DSWIZZLE=6
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),x86-64-gcc430)
ASM_OBJS=misc-x86-64.o mulmod-x86-64.o powmod-x86-64.o btop-x86-64.o
LOOP_OBJS=loop-a64.o loop-a64.p.o loop-core2.o loop-core2.p.o
PATH1_FLAGS+= -DSWIZZLE=4
PATH2_FLAGS+= -DSWIZZLE=6
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),x86-64-osx)
ASM_OBJS=misc-i386.o mulmod-i386.o powmod-sse2.o btop-x86-sse2.o
LOOP_OBJS=loop-x86-sse2.o loop-x86-sse2.p.o
PATH1_FLAGS+= -DSWIZZLE=4
CPPFLAGS+= -DPREFETCH_OPT
endif
ifeq ($(strip $(ARCH)),x86-64-osx)
ASM_OBJS=misc-x86-64.o mulmod-x86-64.o powmod-x86-64.o btop-x86-64.o
LOOP_OBJS=loop-core2.o loop-core2.p.o
PATH1_FLAGS+= -DSWIZZLE=4
CPPFLAGS+= -DPREFETCH_OPT
endif
endif

# Append any user-supplied CFLAGS.
ifeq ($(strip $(DEBUG)),yes)
override CFLAGS:=$(DEBUG_CFLAGS) $(CFLAGS)
else
CPPFLAGS+= -DNDEBUG
override CFLAGS:=$(OPT_CFLAGS) $(CFLAGS)
LDFLAGS+= -s
endif

# Add BOINC flags, if any.
CPPFLAGS += $(BOINC_CPPFLAGS)
LDFLAGS += $(BOINC_LDFLAGS)

#CC=gcc
LDLIBS=-lm $(BOINC_LIBS)

TAR=tar -c
GZIP=gzip -9
ZIP=zip -q -9
MD5SUM=md5sum
SED=sed
FMT=fmt
CMP=cmp

# No changes should be needed below here.

.PHONY: all dist clean realclean

FULL_VER=$(MAJOR_VER).$(MINOR_VER).$(PATCH_VER)

PROGS=gcwsieve

SIEVE_OBJS=gcwsieve.o arithmetic.o begin.o bitmap.o btop-generic.o clock.o \
	cpu.o events.o factors.o files.o loop-generic.o primes.o sieve.o \
	priority.o terms.o threads.o util.o $(ASM_OBJS) $(MULTI_OBJS) \
	$(LOOP_OBJS) $(DEBUG_OBJS) $(EXTRA_OBJS)

SOURCES=Makefile arithmetic.h arithmetic.c begin.c bitmap.h bitmap.c clock.c \
	config.h cpu.c events.c factors.c files.c memset_fast32.h mm_malloc.h \
	primes.c priority.c sieve.c terms.c threads.c gcwsieve.h gcwsieve.c \
	util.c \
	asm-i386-gcc.h asm-i386-msc.h asm-ppc64.c asm-ppc64.h \
	asm-x86-64-gcc.h asm-x86-64-msc.h \
	mulmod-i386.S mulmod-x86-64.S mulmod-ppc64.S misc-i386.S misc-x86-64.S \
	powmod-i386.S powmod-sse2.S powmod-x86-64.S expmod-ppc64.S \
	btop-generic.c btop-x86.S btop-x86-sse2.S btop-x86-64.S btop.h \
	loop-generic.c loop-x86.S loop-x86-sse2.S loop-a64.S loop-core2.S \
        swizzle.h README.txt.in win32/generate.sh win64/generate.sh \
	msc/getopt.h msc/getopt.c msc/stdint.h msc/inttypes.h msc/version.h

# These are generated by generate.sh scripts in win32/64 subdirectories.
# They are not needed for building with MinGW.
#
COFF_OBJS=win32/mulmod-i386.obj win64/mulmod-x86-64.obj win32/misc-i386.obj \
	win64/misc-x86-64.obj win32/powmod-i386.obj win32/powmod-sse2.obj \
	win64/powmod-x86-64.obj win32/btop-x86.obj win32/btop-x86-sse2.obj \
	win64/btop-x86-64.obj win32/loop-x86.obj win32/loop-x86.p.obj \
	win32/loop-x86-sse2.obj win32/loop-x86-sse2.p.obj win64/loop-a64.obj \
	win64/loop-a64.p.obj win64/loop-core2.obj win64/loop-core2.p.obj

TESTS=test/test1.in test/factors1.txt test/README

DOCS=CHANGES COPYING INSTALL README README-smallp README-threads TODO \
	msc/README cwprimes.txt README-boinc

#TARBALL=gcwsieve-$(FULL_VER).tar.gz
TARBALL=gcwsieve-$(FULL_VER)-src.zip

DISTFILES=$(TARBALL) MD5SUM.txt README.txt

all: $(PROGS)

dist: $(DISTFILES)

$(TARBALL): $(DOCS) $(SOURCES) $(COFF_OBJS) $(TESTS)
#	$(TAR) $^ | $(GZIP) > $@
	$(ZIP) $@ $^

MD5SUM.txt: $(TARBALL)
	$(MD5SUM) $^ > $@

README.txt: README.txt.in
	$(SED) -e s/VERSION/$(FULL_VER)/g $< | $(FMT) > $@

gcwsieve: $(SIEVE_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

version.h:
	echo "#define MAJOR_VER $(MAJOR_VER)" > version.h
	echo "#define MINOR_VER $(MINOR_VER)" >> version.h
	echo "#define PATCH_VER $(PATCH_VER)" >> version.h

ASM_H=asm-i386-gcc.h asm-ppc64.h asm-x86-64-gcc.h
arithmetic.o: arithmetic.c arithmetic.h $(ASM_H) config.h
begin.o: begin.c gcwsieve.h config.h bitmap.h arithmetic.h $(ASM_H)
bitmap.o: bitmap.c gcwsieve.h config.h bitmap.h memset_fast32.h $(ASM_H)
clock.o: clock.c gcwsieve.h config.h
cpu.o: cpu.c gcwsieve.h config.h
events.o: events.c gcwsieve.h config.h
factors.o: factors.c gcwsieve.h config.h arithmetic.h $(ASM_H)
files.o: files.c gcwsieve.h config.h
primes.o: primes.c gcwsieve.h bitmap.h config.h memset_fast32.h $(ASM_H)
priority.o: priority.c gcwsieve.h config.h
terms.o: terms.c gcwsieve.h config.h
threads.o: threads.c gcwsieve.h config.h
gcwsieve.o: gcwsieve.c gcwsieve.h config.h version.h
util.o: util.c gcwsieve.h config.h mm_malloc.h
arithmetic-arm.o: arithmetic-arm.S config.h
btop-arm.o: btop-arm.S config.h
loop-arm.o: loop-arm.S config.h
mulmod-i386.o: mulmod-i386.S config.h
mulmod-x86-64.o: mulmod-x86-64.S config.h
powmod-i386.o: powmod-i386.S config.h
powmod-sse2.o: powmod-sse2.S config.h
powmod-x86-64.o: powmod-x86-64.S config.h
asm-ppc64.o: asm-ppc64.c config.h arithmetic.h $(ASM_H)
expmod-ppc64.o: expmod-ppc64.S config.h
mulmod-ppc64.o: mulmod-ppc64.S config.h
btop-x86.o: btop-x86.S config.h
btop-x86-sse2.o: btop-x86-sse2.S config.h
btop-x86-64.o: btop-x86-64.S config.h
loop-x86.o: loop-x86.S config.h
loop-x86-sse2.o: loop-x86-sse2.S config.h
loop-a64.o: loop-a64.S config.h
loop-core2.o: loop-core2.S config.h
misc-i386.o: misc-i386.S config.h
misc-x86-64.o: misc-x86-64.S config.h
revert-loop.o: revert-loop.c gcwsieve.h swizzle.h arithmetic.h


# Prefetch option
#
loop-x86.p.o: loop-x86.S config.h
	$(CC) $(ASFLAGS) $(CPPFLAGS) -DUSE_PREFETCH -c -o $@ $<
loop-x86-sse2.p.o: loop-x86-sse2.S config.h
	$(CC) $(ASFLAGS) $(CPPFLAGS) -DUSE_PREFETCH -c -o $@ $<
loop-a64.p.o: loop-a64.S config.h
	$(CC) $(ASFLAGS) $(CPPFLAGS) -DUSE_PREFETCH -c -o $@ $<
loop-core2.p.o: loop-core2.S config.h
	$(CC) $(ASFLAGS) $(CPPFLAGS) -DUSE_PREFETCH -c -o $@ $<


# Critical code. Compile each file seperately for each code path.
#
sieve.o: sieve.c gcwsieve.h config.h arithmetic.h $(ASM_H) btop.h swizzle.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH1_FLAGS) -c -o $@ $<
sieve2.o: sieve.c gcwsieve.h config.h arithmetic.h $(ASM_H) btop.h swizzle.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH2_FLAGS) -c -o $@ $<

revert-loop.o: revert-loop.c gcwsieve.h config.h arithmetic.h $(ASM_H) swizzle.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH1_FLAGS) -c -o $@ $<
revert-loop2.o: revert-loop.c gcwsieve.h config.h arithmetic.h $(ASM_H) swizzle.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH2_FLAGS) -c -o $@ $<
	
btop-generic.o: btop-generic.c arithmetic.h config.h btop.h $(ASM_H)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH1_FLAGS) -c -o $@ $<
btop-generic2.o: btop-generic.c arithmetic.h config.h btop.h $(ASM_H)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH2_FLAGS) -c -o $@ $<
loop-generic.o: loop-generic.c arithmetic.h config.h swizzle.h $(ASM_H)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH1_FLAGS) -c -o $@ $<
loop-generic2.o: loop-generic.c arithmetic.h config.h swizzle.h $(ASM_H)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH2_FLAGS) -c -o $@ $<
loop-genericp.o: loop-generic.c arithmetic.h config.h swizzle.h $(ASM_H)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH1_FLAGS) -DUSE_PREFETCH -c -o $@ $<
loop-generic2p.o: loop-generic.c arithmetic.h config.h swizzle.h $(ASM_H)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH2_FLAGS) -DUSE_PREFETCH -c -o $@ $<


clean:
	rm -f *.o gcwsieve.log

realclean: clean
	rm -f $(PROGS) version.h $(DISTFILES)
