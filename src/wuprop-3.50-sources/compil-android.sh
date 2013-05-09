#!/bin/bash

cd lib
$CXX $CPPFLAGS $CXXFLAGS -c *.cpp
$CC $CPPFLAGS $CFLAGS -c *.c
cd ..
$CXX $CPPFLAGS $CXXFLAGS $LDFLAGS -I lib/ data_collect.cpp pugixml.cpp lib/*.o -o data_collect $STDCPP

