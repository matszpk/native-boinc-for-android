cd ../client/
$CXX $CXXFLAGS $CPPFLAGS $LDFLAGS -Wall -DONE_PASS_OPT=1 \
    -DVERBOSE -DENABLE_CHECKPOINTING -DFALSE_ONLY -D_BOINC_ \
    -D__STDC_LIMIT_MACROS -I../common/ -I$BOINCROOTDIR/include/boinc \
    -I/boinc/api -I/boinc/lib subset_sum_main.cpp ../common/binary_output.cpp \
    ../common/generate_subsets.cpp \
    -o ../bin/SubsetSum_opt_arm-android-linux-gnu \
    $BOINCROOTDIR/lib/libboinc_api.a $BOINCROOTDIR/lib/libboinc.a $STDCPP
cd ../bin/
