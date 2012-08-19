cd ../client/
$CXX $CXXFLAGS $CPPFLAGS $LDFLAGS -Wall \
    -DVERBOSE -DENABLE_CHECKPOINTING -DFALSE_ONLY -D_BOINC_ \
    -D__STDC_LIMIT_MACROS -I../common/ -I$BOINCROOTDIR/include/boinc \
    -I/boinc/api -I/boinc/lib subset_sum_main.cpp ../common/binary_output.cpp \
    ../common/generate_subsets.cpp \
    -o ../bin/SubsetSum_$1_arm-android-linux-gnu \
    $BOINCROOTDIR/lib/libboinc_api.a $BOINCROOTDIR/lib/libboinc.a $STDCPP
cd ../bin/
