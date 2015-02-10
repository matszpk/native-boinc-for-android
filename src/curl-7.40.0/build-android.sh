#!/bin/sh
./configure --host=arm-linux --prefix="$BOINCROOTDIR" --libdir="$BOINCROOTDIR/lib" --disable-shared --enable-static --with-random=/dev/urandom
make
make install