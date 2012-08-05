# Makefile for sr5sieve. (C) Geoffrey Reynolds, April 2006.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.

MAJOR_VER=1
MINOR_VER=8
PATCH_VER=11

# Set DEBUG=yes to compile with debugging information and internal checks.
#
DEBUG=no

# Set ASSEMBLER=yes to use assembler code where available.
#
ASSEMBLER=yes

# Uncomment one of the following ARCH= lines:
#
#ARCH=ppc64-osx     # Apple G5 with MacOS X
#ARCH=ppc64-linux   # Apple G5 with Linux
#ARCH=i586          # Generic Pentium or compatible
#ARCH=i686          # Pentium 2 or Pentium 3
#ARCH=athlon        # Athlon (32-bit)
#ARCH=pentium4      # Pentium 4 (32-bit)
#ARCH=k8            # Athlon 64 (Also Core 2) in native 64-bit mode, SSE2 only.
#ARCH=k8-fpu        # Athlon 64 (Also Core 2) in native 64-bit mode, x87 only.
#ARCH=x86           # Runs on any i586, i686/Athlon/P4 code paths (32-bit).
#ARCH=x86-64        # Runs on any x86-64, SSE2/x87 code paths (64-bit).
#ARCH=x86-64-gcc430 # Same as ARCH=x86-64 but allow for bugs in GCC 4.3.0.
#ARCH=x86-osx       # Intel x86 with Mac OS X
#ARCH=x86-64-osx    # Intel x86_64 with Mac OS X
ARCH=armv6-android

# If ARCH not set above then these defauts will be used:
#
ifeq ($(strip $(ARCH)),)
DEBUG_CFLAGS=-g
OPT_CFLAGS=-O2
CPPFLAGS=-Wall
ASSEMBLER=no
endif

# Android ARMv6
#
ifeq ($(strip $(ARCH)),armv6-android)
DEBUG_CFLAGS=
OPT_CFLAGS=-O3
CPPFLAGS+=-Wall -DSKIP_VFP_SAVE=1 -DUSE_INLINE_MULMOD -DMULTI_PATH -DARM_CPU=1 -DARM_NOVFP=1 -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
ASFLAGS=
PATH1_FLAGS=-DCODE_PATH=1 -O3
PATH2_FLAGS=-DCODE_PATH=2 -O3 -DARM_NEON=1
#LDFLAGS=
MULTI_OBJS=bsgs2.o
endif


# Apple G5 with OS X
#
ifeq ($(strip $(ARCH)),ppc64-osx)
DEBUG_CFLAGS=-g -O3 -ffast-math -mdynamic-no-pic -mtune=G5 -mcpu=970 -m64
OPT_CFLAGS=-O3 -ffast-math -mdynamic-no-pic -mtune=G5 -mcpu=970 -m64 -fomit-frame-pointer
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mdynamic-no-pic -mtune=G5 -mcpu=970 -m64
LDFLAGS=-m64
endif

# Apple G5 with Linux
#
ifeq ($(strip $(ARCH)),ppc64-linux)
DEBUG_CFLAGS=-g -O3 -ffast-math -mtune=G5 -mcpu=970 -m64 -maltivec -mabi=altivec
OPT_CFLAGS=-O3 -ffast-math -mtune=G5 -mcpu=970 -m64 -fomit-frame-pointer -maltivec -mabi=altivec
CPPFLAGS=-Wall -Xassembler -mregnames -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mregnames -mtune=G5 -mcpu=970 -m64 -maltivec -mabi=altivec
LDFLAGS=-m64
endif

# Pentium compatible
#
ifeq ($(strip $(ARCH)),i586)
DEBUG_CFLAGS=-g -march=i586 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=i586 -m32
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=128
PATH1_FLAGS=
ASFLAGS=-m32
LDFLAGS=-m32
endif

# Intel Pentium2 or Pentium3
#
ifeq ($(strip $(ARCH)),i686)
DEBUG_CFLAGS=-g -march=i686 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=i686 -m32
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
PATH1_FLAGS=
ASFLAGS=-m32
LDFLAGS=-m32
endif

# AMD Athlon (32-bit)
#
ifeq ($(strip $(ARCH)),athlon)
DEBUG_CFLAGS=-g -march=athlon -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=athlon -m32
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
PATH1_FLAGS=-DUSE_CMOV
ASFLAGS=-m32
LDFLAGS=-m32
endif

# Intel Pentium 4 (32-bit)
#
ifeq ($(strip $(ARCH)),pentium4)
DEBUG_CFLAGS=-g -march=pentium4 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=pentium4 -m32
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=512
PATH1_FLAGS=
ASFLAGS=-m32
LDFLAGS=-m32
endif

# AMD Athlon64 in native 64-bit mode. OK for Intel Core2 also.
# Uses cvtsi2sdq/mulsd/cvtsd2siq for mulmod/powmod. (Max p=2^51)
#
ifeq ($(strip $(ARCH)),k8)
DEBUG_CFLAGS=-g -march=k8 -m64
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -m64
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-m64
LDFLAGS=-m64
PATH1_FLAGS=-O3
endif

# AMD Athlon64 in native 64-bit mode. OK for Intel Core2 also.
# Uses fildll/fmul/fistpll for mulmod/powmod. (Max p=2^62)
#
ifeq ($(strip $(ARCH)),k8-fpu)
DEBUG_CFLAGS=-g -march=k8 -m64
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -m64
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DUSE_FPU_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-m64
LDFLAGS=-m64
PATH1_FLAGS=-O3
endif

# Runs on any i586, three critical code paths:
# 1. no-SSE2 no-CMOV, tuned for i686.
# 2. SSE2, no-CMOV, tuned for Pentium 4.
# 3. no-SSE2, CMOV, tuned for Athlon.
#
ifeq ($(strip $(ARCH)),x86)
DEBUG_CFLAGS=-g -march=i586 -mtune=i686 -m32
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=i586 -mtune=i686 -m32
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DMULTI_PATH -DDEFAULT_L1_CACHE_SIZE=16 -DDEFAULT_L2_CACHE_SIZE=256
ASFLAGS=-m32
LDFLAGS=-m32
PATH1_FLAGS=-DCODE_PATH=1 -march=i586 -mtune=i686
PATH2_FLAGS=-DCODE_PATH=2 -march=pentium4 -mtune=i686
PATH3_FLAGS=-DCODE_PATH=3 -DUSE_CMOV -march=athlon -mtune=athlon
MULTI_OBJS=bsgs2.o bsgs3.o
endif

# AMD Athlon64 in native 64-bit mode. OK for Intel Core2 also.
# Uses fildll/fmul/fistpll for mulmod/powmod when p >= 2^51,
# Uses cvtsi2sdq/mulsd/cvtsd2siq for mulmod/powmod when p < 2^51.
#
ifeq ($(strip $(ARCH)),x86-64)
DEBUG_CFLAGS=-g -march=k8 -mno-3dnow -m64
OPT_CFLAGS=-O2 -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -m64
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DUSE_FPU_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512 -DMULTI_PATH
ASFLAGS=-m64
LDFLAGS=-m64
PATH1_FLAGS=-DCODE_PATH=1 -O3
PATH2_FLAGS=-DCODE_PATH=2 -O3 -UUSE_FPU_MULMOD
MULTI_OBJS=bsgs2.o
endif

# Same as ARCH=x86-64 but allow for bugs in GCC 4.3.0.
#
ifeq ($(strip $(ARCH)),x86-64-gcc430)
DEBUG_CFLAGS=-g -march=k8 -mno-3dnow -m64
OPT_CFLAGS=-O -fomit-frame-pointer -ffast-math -march=k8 -mno-3dnow -m64
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DUSE_FPU_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512 -DMULTI_PATH
ASFLAGS=-m64
LDFLAGS=-m64
PATH1_FLAGS=-DCODE_PATH=1
PATH2_FLAGS=-DCODE_PATH=2 -UUSE_FPU_MULMOD
MULTI_OBJS=bsgs2.o
endif

# Intel x86 with OS X
# SSE2 code path only, max p=2^62.
#
ifeq ($(strip $(ARCH)),x86-osx)
DEBUG_CFLAGS=-g -O3 -ffast-math -mdynamic-no-pic -march=prescott -arch i386
OPT_CFLAGS=-O3 -ffast-math -mdynamic-no-pic -march=prescott -arch i386 -fomit-frame-pointer -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
CPPFLAGS=-Wall -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512
ASFLAGS=-mdynamic-no-pic -march=prescott -arch i386
LDFLAGS=-arch i386 -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
endif

# Intel x86_64 with OS X
# Uses fildll/fmul/fistpll for mulmod/powmod when p >= 2^51, max p=2^62.
# Uses cvtsi2sdq/mulsd/cvtsd2siq for mulmod/powmod when p < 2^51.
#
ifeq ($(strip $(ARCH)),x86-64-osx)
DEBUG_CFLAGS=-g -O3 -ffast-math -mdynamic-no-pic -march=nocona -arch x86_64
OPT_CFLAGS=-O3 -ffast-math -march=nocona -arch x86_64 -fomit-frame-pointer -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
CPPFLAGS=-Wall -DUSE_INLINE_MULMOD -DUSE_FPU_MULMOD -DDEFAULT_L1_CACHE_SIZE=32 -DDEFAULT_L2_CACHE_SIZE=512 -DMULTI_PATH
ASFLAGS=-mdynamic-no-pic -march=nocona -arch x86_64
LDFLAGS=-arch x86_64 -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
PATH1_FLAGS=-DCODE_PATH=1
PATH2_FLAGS=-DCODE_PATH=2 -UUSE_FPU_MULMOD
MULTI_OBJS=bsgs2.o
endif


# Add ARCH-specific assembler objects and flags.
#
ifeq ($(strip $(ASSEMBLER)),yes)
CPPFLAGS+= -DUSE_ASM
ASM_OBJS=
CHECK_OBJS=
ifeq ($(strip $(ARCH)),armv6-android)
ASM_OBJS=arithmetic-arm.o arithmetic-neon.o hash-arm.o giant-arm.o giant-neon.o \
	powmod-arm.o powmod-neon.o powseq-arm.o powseq-neon.o \
	search-elim-arm.o setup64-arm.o
CHECK_OBJS=arithmetic-arm.o
endif
ifeq ($(strip $(ARCH)),ppc64-osx)
CPPFLAGS+= -DNEED_UNDERSCORE
ASM_OBJS=asm-ppc64.o expmod-ppc64.o mulmod-ppc64.o
CHECK_OBJS=asm-ppc64.o expmod-ppc64.o mulmod-ppc64.o
endif
ifeq ($(strip $(ARCH)),ppc64-linux)
ASM_OBJS=asm-ppc64.o expmod-ppc64.o mulmod-ppc64.o
CHECK_OBJS=asm-ppc64.o expmod-ppc64.o mulmod-ppc64.o
endif
ifeq ($(strip $(ARCH)),i586)
ASM_OBJS=mulmod-i386.o powmod-i386.o hash-i386.o misc-i386.o lookup-i386.o
CHECK_OBJS=mulmod-i386.o powmod-i386.o
endif
ifeq ($(strip $(ARCH)),i686)
ASM_OBJS=mulmod-i386.o powmod-i386.o hash-i386.o misc-i386.o lookup-i386.o
CHECK_OBJS=mulmod-i386.o powmod-i386.o
endif
ifeq ($(strip $(ARCH)),athlon)
ASM_OBJS=mulmod-i386.o powmod-i386.o hash-i386.o misc-i386.o lookup-i386.o
CHECK_OBJS=mulmod-i386.o powmod-i386.o
endif
ifeq ($(strip $(ARCH)),pentium4)
ASM_OBJS=mulmod-i386.o powmod-sse2.o mulmod-sse2.o hash-i386.o misc-i386.o \
	lookup-sse2.o
CHECK_OBJS=mulmod-i386.o powmod-sse2.o mulmod-sse2.o
endif
ifeq ($(strip $(ARCH)),k8)
ASM_OBJS=powmod-x86-64.o mulmod-x86-64.o hash-x86-64.o giant-x86-64.o \
	misc-x86-64.o lookup-x86_64.o
CHECK_OBJS=powmod-x86-64.o mulmod-x86-64.o
endif
ifeq ($(strip $(ARCH)),k8-fpu)
ASM_OBJS=powmod-x87-64.o mulmod-x87-64.o hash-x86-64.o giant-x87-64.o \
	misc-x86-64.o lookup-x87_64.o
CHECK_OBJS=powmod-x87-64.o mulmod-x87-64.o
endif
ifeq ($(strip $(ARCH)),x86)
ASM_OBJS=powmod-i386.o mulmod-i386.o powmod-sse2.o mulmod-sse2.o hash-i386.o \
	misc-i386.o lookup-i386.o lookup-sse2.o
CHECK_OBJS=powmod-i386.o mulmod-i386.o powmod-sse2.o mulmod-sse2.o
endif
ifeq ($(strip $(ARCH)),x86-64)
ASM_OBJS=powmod-x86-64.o powmod-x87-64.o mulmod-x86-64.o mulmod-x87-64.o \
	 hash-x86-64.o giant-x86-64.o giant-x87-64.o misc-x86-64.o \
	 lookup-x86_64.o lookup-x87_64.o
CHECK_OBJS=powmod-x86-64.o powmod-x87-64.o mulmod-x86-64.o mulmod-x87-64.o
endif
ifeq ($(strip $(ARCH)),x86-64-gcc430)
ASM_OBJS=powmod-x86-64.o powmod-x87-64.o mulmod-x86-64.o mulmod-x87-64.o \
	 hash-x86-64.o giant-x86-64.o giant-x87-64.o misc-x86-64.o \
	 lookup-x86_64.o lookup-x87_64.o
CHECK_OBJS=powmod-x86-64.o powmod-x87-64.o mulmod-x86-64.o mulmod-x87-64.o
endif
ifeq ($(strip $(ARCH)),x86-osx)
ASM_OBJS=mulmod-i386.o powmod-sse2.o mulmod-sse2.o hash-i386.o misc-i386.o \
	lookup-sse2.o
CHECK_OBJS=mulmod-i386.o powmod-sse2.o mulmod-sse2.o
endif
ifeq ($(strip $(ARCH)),x86-64-osx)
ASM_OBJS=powmod-x86-64.o powmod-x87-64.o mulmod-x86-64.o mulmod-x87-64.o \
	 hash-x86-64.o giant-x86-64.o giant-x87-64.o misc-x86-64.o \
	 lookup-x86_64.o lookup-x87_64.o
CHECK_OBJS=powmod-x86-64.o powmod-x87-64.o mulmod-x86-64.o mulmod-x87-64.o
endif
endif

# Append any user-supplied CFLAGS.
#
ifeq ($(strip $(DEBUG)),yes)
override CFLAGS:=$(DEBUG_CFLAGS) $(CFLAGS)
else
CPPFLAGS+= -DNDEBUG
override CFLAGS:=$(OPT_CFLAGS) $(CFLAGS)
LDFLAGS+= -s
endif

override CC:=$(CC) $(CFLAGS)

#CPPFLAGS+= -DDUMP_ITER_STATE=1

LDLIBS=-lm

TAR=tar -c
GZIP=gzip -9
ZIP=zip -q -9
MD5SUM=md5sum
SED=sed
FMT=fmt
CMP=cmp

# No changes should be needed below here.

.PHONY: all check dist clean realclean

FULL_VER=$(MAJOR_VER).$(MINOR_VER).$(PATCH_VER)

PROGS=sr5sieve

SR5SIEVE_OBJS=sr5sieve.o arithmetic.o bitmap.o bsgs.o choose.o clock.o \
	cpu.o events.o factors.o files.o hashtable.o legendre.o primes.o \
	priority.o sequences.o sobistrator.o subseq.o threads.o util.o \
	$(ASM_OBJS) $(MULTI_OBJS)

SRTEST_OBJS=srtest.o arithmetic.o $(CHECK_OBJS)
SRTEST_LIBS=-lgmp

SOURCES=Makefile arithmetic.h arithmetic.c bitmap.h bitmap.c bsgs.c choose.c \
	clock.c config.h cpu.c events.c factors.c files.c hashtable.h \
	hashtable.c legendre.c memset_fast32.h mm_malloc.h primes.c \
	priority.c sequences.c sr5sieve.h sr5sieve.c sobistrator.c subseq.c \
	threads.c util.c srtest.c misc-i386.S misc-x86-64.S \
	asm-i386-gcc.h asm-ppc64.c asm-ppc64.h asm-x86-64-gcc.h \
	mulmod-i386.S powmod-i386.S powmod-sse2.S mulmod-sse2.S \
	powmod-x86-64.S powmod-x87-64.S mulmod-x86-64.S mulmod-x87-64.S \
	hash-i386.S hash-x86-64.S giant-x86-64.S giant-x87-64.S \
        expmod-ppc64.S mulmod-ppc64.S mulmod-ppc64.c lookup-ind.h \
	lookup-x86_64.S lookup-x87_64.S lookup-i386.S lookup-sse2.S \
	arithmetic-arm.S hash-arm.S giant-arm.S powmod-arm.S powseq-arm.S \
	search-elim-arm.S setup64-arm.S arithmetic-neon.S giant-neon.S \
	powmod-neon.S powseq-neon.S README.txt.in

DOCS=CHANGES COPYING INSTALL README5 README2 README-threads TODO README-haiku

#TARBALL=sr5sieve-$(FULL_VER).tar.gz
TARBALL=sr5sieve-$(FULL_VER)-src.zip

DISTFILES=$(TARBALL) MD5SUM.txt README.txt

all: sr5sieve

dist: $(DISTFILES)

check: srtest
	./srtest 1000000 0
	@echo "All tests passed."

$(TARBALL): $(DOCS) $(SOURCES)
#	$(TAR) $^ | $(GZIP) > $@
	$(ZIP) $@ $^

MD5SUM.txt: $(TARBALL)
	$(MD5SUM) $^ > $@

README.txt: README.txt.in
	$(SED) -e s/VERSION/$(FULL_VER)/g $< | $(FMT) > $@

sr5sieve: $(SR5SIEVE_OBJS)

srtest: $(SRTEST_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(SRTEST_LIBS)

version.h:
	echo "#define MAJOR_VER $(MAJOR_VER)" > version.h
	echo "#define MINOR_VER $(MINOR_VER)" >> version.h
	echo "#define PATCH_VER $(PATCH_VER)" >> version.h

ASM_H=asm-i386-gcc.h asm-ppc64.h asm-x86-64-gcc.h

arithmetic.o: arithmetic.c arithmetic.h $(ASM_H) config.h
arithmetic-arm.o: arithmetic-arm.S config.h
arithmetic-neon.o: arithmetic-neon.S config.h
bitmap.o: bitmap.c sr5sieve.h config.h bitmap.h memset_fast32.h $(ASM_H)
choose.o: choose.c sr5sieve.h config.h
clock.o: clock.c sr5sieve.h config.h
cpu.o: cpu.c sr5sieve.h config.h
events.o: events.c sr5sieve.h config.h
factors.o: factors.c sr5sieve.h config.h arithmetic.h $(ASM_H)
files.o: files.c sr5sieve.h config.h
hashtable.o: hashtable.c sr5sieve.h config.h hashtable.h memset_fast32.h $(ASM_H)
legendre.o: legendre.c sr5sieve.h config.h bitmap.h
primes.o: primes.c sr5sieve.h bitmap.h config.h memset_fast32.h $(ASM_H)
priority.o: priority.c sr5sieve.h config.h
sequences.o: sequences.c sr5sieve.h config.h
sr5sieve.o: sr5sieve.c sr5sieve.h config.h version.h
srtest.o: srtest.c sr5sieve.h config.h version.h arithmetic.h $(ASM_H)
sobistrator.o: sobistrator.c sr5sieve.h config.h
subseq.o: subseq.c sr5sieve.h config.h bitmap.h
threads.o: threads.c sr5sieve.h config.h
util.o: util.c sr5sieve.h config.h mm_malloc.h
powmod-arm.o: powmod-arm.S config.h
powmod-neon.o: powmod-neon.S config.h
mulmod-i386.o: mulmod-i386.S config.h
powmod-i386.o: powmod-i386.S config.h
mulmod-sse2.o: mulmod-sse2.S config.h
powmod-sse2.o: powmod-sse2.S config.h
powmod-x86-64.o: powmod-x86-64.S config.h
powmod-x87-64.o: powmod-x87-64.S config.h
powseq-arm.o: powseq-arm.S config.h
powseq-neon.o: powseq-neon.S config.h
hash-arm.o: hash-arm.S config.h hashtable.h
hash-i386.o: hash-i386.S config.h hashtable.h
hash-x86-64.o: hash-x86-64.S config.h hashtable.h
giant-arm.o: giant-arm.S config.h hashtable.h
giant-neon.o: giant-neon.S config.h hashtable.h
giant-x86-64.o: giant-x86-64.S config.h hashtable.h
giant-x87-64.o: giant-x87-64.S config.h hashtable.h
asm-ppc64.o: asm-ppc64.c config.h arithmetic.h $(ASM_H)
mulmod-ppc64.o: mulmod-ppc64.c config.h arithmetic.h
expmod-ppc64.o: expmod-ppc64.S config.h
mulmod-x86-64.o: mulmod-x86-64.S config.h
mulmod-x87-64.o: mulmod-x87-64.S config.h
misc-i386.o: misc-i386.S config.h
misc-x86-64.o: misc-x86-64.S config.h
lookup-x86_64.o: lookup-x86_64.S config.h lookup-ind.h
lookup-x87_64.o: lookup-x87_64.S config.h lookup-ind.h
lookup-i386.o: lookup-i386.S config.h lookup-ind.h
lookup-sse2.o: lookup-sse2.S config.h lookup-ind.h
search-elim-arm.o: search-elim-arm.S config.h hashtable.h


# Critical code. Compile each file seperately for each code path.
#
bsgs.o: bsgs.c sr5sieve.h config.h arithmetic.h $(ASM_H) bitmap.h hashtable.h lookup-ind.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH1_FLAGS) -c -o $@ $<
bsgs2.o: bsgs.c sr5sieve.h config.h arithmetic.h $(ASM_H) bitmap.h hashtable.h lookup-ind.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH2_FLAGS) -c -o $@ $<
bsgs3.o: bsgs.c sr5sieve.h config.h arithmetic.h $(ASM_H) bitmap.h hashtable.h lookup-ind.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PATH3_FLAGS) -c -o $@ $<


clean:
	rm -f *.o sr?sieve.log checkpoint.txt factors*.txt srtest

realclean: clean
	rm -f $(PROGS) version.h $(DISTFILES)
