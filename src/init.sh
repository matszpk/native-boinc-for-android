# path to Android NDK
NDKPATH=~/docs/dev/mobile/android-ndk-r5b
# path to Android SDK
SDKPATH=~/docs/dev/mobile/android-sdk-linux_x86

export SYSROOT=$NDKPATH/platforms/android-8/arch-arm/
export NDKTOOLPATH=$NDKPATH/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/
export PATH="$PATH:$SDKPATH:$SDKPATH/platform-tools:$NDKTOOLPATH"
export CC=$NDKTOOLPATH/arm-linux-androideabi-gcc
export CXX=$NDKTOOLPATH/arm-linux-androideabi-g++
export LD=$NDKTOOLPATH/arm-linux-androideabi-ld

#export CFLAGS="--sysroot=$SYSROOT -Wall -I/data/local/tmp/include"
# uncomment when you compile boinc-distrib and dependency libs
#export CFLAGS="--sysroot=$SYSROOT -Wall -I/data/local/tmp/include -O3 -mthumb"
# for debugging
export GDB_CFLAGS="--sysroot=$SYSROOT -Wall -g -I/data/local/tmp/include"
# uncomment when you compile boinc-distrib and dependency libs
export CXXFLAGS="--sysroot=$SYSROOT -Wall -funroll-loops -fexceptions -O3 -mthumb"
# uncomment when you compile boinc application
#export CXXFLAGS="--sysroot=$SYSROOT -Wall -funroll-loops -fexceptions"
export LDFLAGS="-L$SYSROOT/usr/lib -L/data/local/tmp/lib"
export CPPFLAGS="-I/data/local/tmp/include -DANDROID=1 -I$NDKPATH/sources/cxx-stl/gnu-libstdc++/include/ -I$NDKPATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include/"
export STDCPP=$NDKPATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi/libstdc++.a
export ANDROID_NDK_ROOT=$NDKPATH