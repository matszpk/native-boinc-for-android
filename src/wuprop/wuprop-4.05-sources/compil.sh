#!/bin/bash

cd lib
g++-4.4 -c mfile.cpp app_ipc.cpp gui_rpc_client.cpp boinc_api.cpp gui_rpc_client_ops.cpp util.cpp hostinfo.cpp str_util.cpp prefs.cpp parse.cpp miofile.cpp proxy_info.cpp url.cpp network.cpp md5_file.cpp shmem.cpp diagnostics.cpp proc_control.cpp coproc.cpp notice.cpp cc_config.cpp md5.c procinfo_unix.cpp filesys.cpp procinfo.cpp
ar -rc libboinc.a *.o
cd ..
g++-4.4 -I lib/ *.cpp -pthread lib/libboinc.a -ldl -o data_collect

