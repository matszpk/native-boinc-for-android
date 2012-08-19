cd ../client/
/usr/bin/g++-4.0 -DVERBOSE -DENABLE_CHECKPOINTING -D_BOINC_ -DFALSE_ONLY -D__STDC_LIMIT_MACROS -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fvisibility=hidden -O3 -msse3 -Wall -mfpmath=sse -mtune=prescott -ftree-vectorize -funroll-loops -I/Users/deselt/Software/boinc -I/Users/deselt/Software/boinc/api -I/Users/deselt/Software/boinc/lib -L/Users/deselt/Software/boinc/mac_build/build/Deployment -lboinc_api -lboinc subset_sum_main.cpp ../common/binary_output.cpp -o ../bin/SubsetSum_$1_i686-apple-darwin
cd ../bin/
