#!/bin/bash

cd lib
g++ -c *.cpp *.c
ar -rc libboinc.a *.o
cd ..
g++ -I lib/ data_collect.cpp pugixml.cpp -pthread lib/libboinc.a -o data_collect

