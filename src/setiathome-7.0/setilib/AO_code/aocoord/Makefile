#
SHELL=/bin/sh
#
# where the source code is
#
SRCDIR = .
#
# where the library will be
#
LIBDIR = $(SRCDIR)
CC=gcc
INCLUDES = -I$(SRCDIR)
ARFLAGS=rvs
CFLAGS=-O -Wall $(INCLUDES)
RM=@rm -f
#
# name of the library (for the -l )
#
LIBNM=azzatoradec
#
# full lib path
#
LIBP=$(LIBDIR)/lib$(LIBNM).a
#
OBJ=\
M3D_Transpose.o \
MM3D_Mult.o \
MV3D_Mult.o \
V3D_Normalize.o \
VV3D_Sub.o \
aberAnnual_V.o \
anglesToVec3.o \
azElToHa_V.o \
azzaToRaDec.o \
azzaToRaDecInit.o \
dms3_rad.o \
dmToDayNo.o \
fmtDms.o \
fmtHmsD.o \
fmtRdToDms.o \
fmtRdToHmsD.o \
fmtSMToHmsD.o \
gregToMjd.o \
gregToMjdDno.o \
haToAzEl_V.o \
haToRaDec_V.o \
hms3_rad.o \
isLeapYear.o \
meanEqToEcl_A.o \
mjdToJulDate.o \
nutation_M.o \
obsPosInp.o \
precJ2ToDate_M.o \
precNut.o \
precNutInit.o \
rad_dms3.o \
rad_hms3.o \
rotationX_M.o \
rotationY_M.o  \
rotationZ_M.o \
secMid_hms3.o \
setSign.o \
truncDtoI.o \
ut1ToLmst.o \
utcInfoInp.o \
utcToUt1.o \
vec3ToAngles.o \
#
.PRECIOUS:$(LIBP)
.INIT:
	echo "$(LIBNM)"

testazzatoradec: $(LIBP)  azzaToRaDec.h
	gcc $(CFLAGS) testazzatoradec.c -L$(LIBDIR) -l$(LIBNM) -lm \
		-o testazzatoradec

lib:$(LIBP)
$(LIBP): $(OBJ)
	$(AR) $(ARFLAGS) $(LIBP) $(OBJ)
#	@$(RM) *.o
	@echo "$(LIBP) rebuild complete"

