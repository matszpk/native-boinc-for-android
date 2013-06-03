#!/bin/sh

# Berkeley Open Infrastructure for Network Computing
# http://boinc.berkeley.edu
# Copyright (C) 2005 University of California
#
# This is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# either version 2.1 of the License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# To view the GNU Lesser General Public License visit
# http://www.gnu.org/copyleft/lesser.html
# or write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
# Script to build Macintosh Universal Binary library of fftw-3.3.1 for
# use in building SETI_BOINC.
#
# by Charlie Fenton 5/2/06
# updated for OS 10.4 SDK and SETI@home v7 on 4/9/11
#
## In Terminal, CD to the fftw-3.3.1 directory.
##     cd [path]/lib/fftw-3.3.1/
## then run this script:
##     source [path]/buildfftw-3.3.1 [ -clean ]
##
## the -clean argument will force a full rebuild.
#

if [ "$1" != "-clean" ]; then
  if [ -f .libs/libfftw3f_ppc.a ] && [ -f .libs/libfftw3f_i386.a ] && [ -f .libs/libfftw3f.a ]; then
    
    echo "buildfftw-3.3.1 already built"
    return 0
  fi
fi

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export SDKROOT="/Developer/SDKs/MacOSX10.4u.sdk"

export CFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -mcpu=powerpc -mtune=G5 -arch ppc -D_THREAD_SAFE -mmacosx-version-min=10.4"
export CPPFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -mcpu=powerpc -mtune=G5 -arch ppc -D_THREAD_SAFE -mmacosx-version-min=10.4"
export LDFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -arch ppc"

./configure -C --with-pic --enable-altivec  --enable-fma --disable-fortran --enable-portable-binary --enable-single --enable-threads --with-combined-threads --host=ppc --build=ppc
if [  $? -ne 0 ]; then exit 1; fi

make clean

rm -f .libs/libfftw3f.a
rm -f .libs/libfftw3f_ppc.a
rm -f .libs/libfftw3f_i386.a


make -e
if [  $? -ne 0 ]; then exit 1; fi
mv -f .libs/libfftw3f.a ./libfftw3f_ppc.a
    
make clean
if [  $? -ne 0 ]; then exit 1; fi

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export LDFLAGS=""
export CPPFLAGS=""
export CFLAGS=""
export SDKROOT="/Developer/SDKs/MacOSX10.4u.sdk"

export CFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -march=pentium-m -mtune=pentium-m -mfpmath=sse -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch i386"
export CPPFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -march=pentium-m -mtune=pentium-m -mfpmath=sse -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch i386"
export LDFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -arch i386"

./configure --enable-sse --disable-fortran --enable-portable-binary --enable-single --enable-threads --with-combined-threads --host=i386
if [  $? -ne 0 ]; then exit 1; fi

make -e
if [  $? -ne 0 ]; then exit 1; fi
mv -f .libs/libfftw3f.a .libs/libfftw3f_i386.a
mv -f ./libfftw3f_ppc.a .libs/libfftw3f_ppc.a
lipo -create .libs/libfftw3f_i386.a .libs/libfftw3f_ppc.a -output .libs/libfftw3f.a
if [  $? -ne 0 ]; then exit 1; fi

export CC="";export CXX=""
export LDFLAGS=""
export CPPFLAGS=""
export CFLAGS=""
export SDKROOT=""

return 0
