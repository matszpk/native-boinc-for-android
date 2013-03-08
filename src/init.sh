# path to Android NDK
NDKPATH=~/docs/dev/mobile/android-ndk-r5b
# path to Android SDK
SDKPATH=~/docs/dev/mobile/android-sdk-linux_x86
# installation (destination) directory
export BOINCROOTDIR=~/docs/src/android/script_compile/root

export SYSROOT=$NDKPATH/platforms/android-4/arch-arm/
export NDKTOOLPATH=$NDKPATH/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/
export PATH="$PATH:$SDKPATH:$SDKPATH/platform-tools:$NDKTOOLPATH"
export CC=$NDKTOOLPATH/arm-linux-androideabi-gcc
export CXX=$NDKTOOLPATH/arm-linux-androideabi-g++
export LD=$NDKTOOLPATH/arm-linux-androideabi-ld

#export CFLAGS="--sysroot=$SYSROOT -Wall"
#export CFLAGS="--sysroot=$SYSROOT -Wall -O3 -mhard-float -mfpu=vfp -mfloat-abi=softfp -fomit-frame-pointer"
#export CFLAGS="--sysroot=$SYSROOT -Wall -O3 -mhard-float -march=armv7-a -mfpu=neon -mtune=cortex-a9 -mfloat-abi=softfp -fomit-frame-pointer"
export CFLAGS="--sysroot=$SYSROOT -Wall -O3 -fomit-frame-pointer"
#export CFLAGS="--sysroot=$SYSROOT -Wall -g"

export GDB_CFLAGS="--sysroot=$SYSROOT -Wall -g"
#export CXXFLAGS="--sysroot=$SYSROOT -Wall -funroll-loops -fexceptions"
#export CXXFLAGS="--sysroot=$SYSROOT -Wall -O3 -mhard-float -mfpu=vfp -mfloat-abi=softfp -fexceptions -fomit-frame-pointer"
#export CXXFLAGS="--sysroot=$SYSROOT -Wall -O3 -mhard-float -march=armv7-a -mfpu=neon -mtune=cortex-a9 -mfloat-abi=softfp -fexceptions -fomit-frame-pointer"
export CXXFLAGS="--sysroot=$SYSROOT -Wall -funroll-loops -fexceptions -O3 -fomit-frame-pointer"
#export CXXFLAGS="--sysroot=$SYSROOT -Wall -g -fno-rtti -fno-exceptions"

export LDFLAGS="-L$SYSROOT/usr/lib -L$BOINCROOTDIR/lib"

export CPPFLAGS="-I$DESTDIR/include -DANDROID=1 -I$NDKPATH/sources/cxx-stl/gnu-libstdc++/include/ -I$NDKPATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include/ -I$BOINCROOTDIR/include"
export STDCPP="$NDKPATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi/libstdc++.a"
#export CPPFLAGS="-I$BOINCROOTDIR/include -DANDROID=1 -I$NDKPATH/sources/cxx-stl/gnu-libstdc++/include/ -I$NDKPATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/include/"
#export STDCPP="$NDKPATH/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a/libstdc++.a"
#export ANDROID_NDK_ROOT=~/docs/dev/mobile/android-ndk-r5b/