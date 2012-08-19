cd ../client/
g++ -DVERBOSE -DENABLE_CHECKPOINTING -DFALSE_ONLY -D_BOINC_ -D__STDC_LIMIT_MACROS -O2 -msse2 -m32 -ftree-vectorize -funroll-loops -Wall -static-libgcc -I/home/tdesell/Software/boinc -I/home/tdesell/Software/boinc/api -I/home/tdesell/Software/boinc/lib subset_sum_main.cpp ../common/binary_output.cpp -o ../bin/SubsetSum_$1_i686-pc-linux-gnu -L/home/tdesell/Software/boinc/lib -L/home/tdesell/Software/boinc/api /usr/lib/gcc/i686-linux-gnu/4.6.1/libstdc++.so -lboinc_api -lboinc -pthread
cd ../bin/
