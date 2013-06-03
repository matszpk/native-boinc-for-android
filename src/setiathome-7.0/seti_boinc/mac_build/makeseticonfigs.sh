#!/bin/sh

# Berkeley Open Infrastructure for Network Computing
# http://boinc.berkeley.edu
# Copyright (C) 2006 University of California
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
# Script to build PowerPC-specific and Intel-specific config.h files
# for use in building SETI_BOINC.
#
# by Charlie Fenton 9/10/07
# updated for OS 10.4 SDK and SETI@home v7 on 4/9/11
#
## In Terminal, CD to the seti_boinc directory.
##     cd [path]/seti_boinc/
## then run this script:
##     source [path]/makeseticonfigs.sh path_to_seti_boinc_dir path_to_boinc_dir
#

if [ $# -ne 2 ]; then
     echo "usage sh $0 path_to_seti_boinc_dir path_to_boinc_dir"
     exit
fi

echo "  *************************************************"
echo "  *                                               *"
echo "  *        Configuring for i686-apple-darwin      *"
echo "  *                                               *"
echo "  *************************************************"

cd "$1" 
##export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export SDKROOT="/Developer/SDKs/MacOSX10.4u.sdk"

export CFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch i386"
export CPPFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch i386"

export PROJECTDIR="$1"
export BOINCDIR="$2"
./_autosetup --host=i686-apple-darwin


if [ $? -ne 0 ]; then
    exit
fi

./configure -C --disable-server --enable-fast-math --enable-sse3 --with-apple-opengl-framework --disable-dynamic-graphics --host=i686-apple-darwin

if [ $? -ne 0 ]; then
    exit
fi

mv ./sah_config.h ./mac_build/config-i386.h


echo "  *************************************************"
echo "  *                                               *"
echo "  *      Configuring for powerpc-apple-darwin     *"
echo "  *                                               *"
echo "  *************************************************"

cd "$1" 
##export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export SDKROOT="/Developer/SDKs/MacOSX10.4u.sdk"

export CFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -arch ppc -D_THREAD_SAFE -mmacosx-version-min=10.4"
export CPPFLAGS="-isysroot/Developer/SDKs/MacOSX10.4u.sdk -O3 -arch ppc -D_THREAD_SAFE -mmacosx-version-min=10.4"

export PROJECTDIR="$1"
export BOINCDIR="$2"
./_autosetup --host=powerpc-apple-darwin

if [ $? -ne 0 ]; then
    exit
fi

./configure --disable-server --enable-fast-math --enable-altivec --with-apple-opengl-framework --disable-dynamic-graphics --host=powerpc-apple-darwin

if [ $? -ne 0 ]; then
    exit
fi

mv ./sah_config.h ./mac_build/config-ppc.h


