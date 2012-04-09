#!/bin/sh
./_autosetup
./configure --host=arm-linux --prefix="$BOINCROOTDIR" --libdir="$BOINCROOTDIR/lib" --with-boinc-platform=arm-android-linux-gnu \
    --with-boinc-alt-platform=arm-android --disable-server --disable-manager --disable-shared --enable-static

# prepare Makefiles (for benchmarks)
patch client/Makefile client/Makefile-android.patch
sed -e "s%^CLIENTLIBS *= *.*$%CLIENTLIBS = -lm $STDCPP%g" client/Makefile > client/Makefile.out
mv client/Makefile.out client/Makefile

make
