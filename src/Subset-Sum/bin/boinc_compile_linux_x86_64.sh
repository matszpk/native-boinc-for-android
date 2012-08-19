cd ../client/
g++ -m64 -O3 -msse3 -ftree-vectorize -funroll-loops -static-libgcc -Wall -DVERBOSE -DENABLE_CHECKPOINTING -DFALSE_ONLY -D_BOINC_ -D__STDC_LIMIT_MACROS -I/boinc -I/boinc/api -I/boinc/lib subset_sum_main.cpp ../common/binary_output.cpp -o ../bin/SubsetSum_$1_x86_64-pc-linux-gnu -L/boinc/lib -L/boinc/api /usr/lib/gcc/x86_64-redhat-linux/4.4.6/libstdc++.a -lboinc_api -lboinc -pthread
cd ../bin/
