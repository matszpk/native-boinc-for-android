t1.s: ../config.h ../../boinc/lib/std_fixes.h simd.h mmx.h generics.h t1.cpp sse.h
	gcc -w -I../../boinc/lib -march=pentium4 -mfpmath=sse -O2 -DUSE_SSE -S t1.cpp

t1: ../config.h ../../boinc/lib/std_fixes.h simd.h mmx.h generics.h t1.cpp sse.h
	gcc  -w -I../../boinc/lib -march=pentium4 -mfpmath=sse -O2 -DUSE_SSE -o t1 t1.cpp

ps.s: ../config.h ../../boinc/lib/std_fixes.h simd.h mmx.h generics.h ps.cpp sse.h
	gcc -w -I../../boinc/lib -march=pentium4 -mfpmath=sse -O2 -DUSE_SSE -S ps.cpp

