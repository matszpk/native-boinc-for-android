export BOINCDIR=/home/mat/docs/src/android/native-boinc-for-android/src/boinc-6.12.38/
export CFLAGS="$CFLAGS $CPPFLAGS"
export CXXFLAGS="$CXXFLAGS $CPPFLAGS"
./_autosetup
./configure --build=x86_64-unknown-linux-gnu --host=arm-linux --prefix=$BOINCROOTDIR  \
    --libdir=$BOINCROOTDIR/lib --disable-server --disable-graphics --enable-armopts --enable-pffft
make