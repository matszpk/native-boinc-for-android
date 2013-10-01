#!/bin/bash

cd lib
$CXX $CPPFLAGS $CXXFLAGS -c mfile.cpp app_ipc.cpp gui_rpc_client.cpp boinc_api.cpp gui_rpc_client_ops.cpp util.cpp hostinfo.cpp str_util.cpp prefs.cpp parse.cpp miofile.cpp proxy_info.cpp url.cpp network.cpp md5_file.cpp shmem.cpp diagnostics.cpp proc_control.cpp coproc.cpp notice.cpp cc_config.cpp md5.c procinfo_unix.cpp filesys.cpp procinfo.cpp
$AR -rc libboinc.a *.o
cd ..
$CXX $CPPFLAGS $CXXFLAGS $LDFLAGS -I lib/ *.cpp lib/libboinc.a -ldl -o data_collect $STDCPP

