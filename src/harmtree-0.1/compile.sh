#!/bin/sh
$CXX $CXXFLAGS $CPPFLAGS $LDFLAGS -I$BOINCROOTDIR/include/boinc \
	-o HarmoniousB ../HarmoniousB.cpp $BOINCROOTDIR/lib/libboinc_api.a \
	$BOINCROOTDIR/lib/libboinc.a $STDCPP