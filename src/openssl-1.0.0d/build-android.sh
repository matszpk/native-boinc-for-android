#!/bin/sh
./Configure linux-generic32 no-shared no-dso -DL_ENDIAN --openssldir="$BOINCROOTDIR/ssl"

sed -e "s/^CFLAG=.*$/`grep -e \^CFLAG= Makefile` \$(CFLAGS)/g
s%^INSTALLTOP=.*%INSTALLTOP=$BOINCROOTDIR%g" Makefile > Makefile.out
mv Makefile.out Makefile
make
# install do dest directory
make install_sw
