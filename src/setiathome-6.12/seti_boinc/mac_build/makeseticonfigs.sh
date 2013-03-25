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
export PATH=/usr/local/bin:$PATH
CPPFLAGS="-arch i386";export CPPFLAGS
export PROJECTDIR="$1"
export BOINCDIR="$2"
./_autosetup --host=i686-apple-darwin


if [ $? -ne 0 ]; then
    exit
fi

./configure --disable-server --with-apple-opengl-framework --disable-dynamic-graphics --host=i686-apple-darwin

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
export PATH=/usr/local/bin:$PATH
export NEXT_ROOT=/Developer/SDKs/MacOSX10.3.9.sdk
export CPPFLAGS="-arch ppc -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include -DMAC_OS_X_VERSION_MAX_ALLOWED=1030"
export LDFLAGS="-arch ppc -L/Developer/SDKs/MacOSX10.3.9.sdk/usr/lib/gcc/darwin/3.3"
export PROJECTDIR="$1"
export BOINCDIR="$2"
export CC=/usr/bin/gcc-3.3;export CXX=/usr/bin/g++-3.3
./_autosetup --host=powerpc-apple-darwin

if [ $? -ne 0 ]; then
    exit
fi

./configure --disable-server --with-apple-opengl-framework --disable-dynamic-graphics --host=powerpc-apple-darwin

if [ $? -ne 0 ]; then
    exit
fi

mv ./sah_config.h ./mac_build/config-ppc.h


