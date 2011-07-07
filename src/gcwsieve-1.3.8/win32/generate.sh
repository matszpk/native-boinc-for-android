#/bin/sh
#
# Generate COFF32 objects from GAS x86 assembly.
# objconv from http://www.agner.org/optimize.
#
for FILE in \
    mulmod-i386 \
    misc-i386 \
    powmod-i386 powmod-sse2 \
    btop-x86 btop-x86-sse2 \
    loop-x86 loop-x86-sse2 \
    ;
  do gcc -m32 -Wall -DASSEMBLE_FOR_MSC -I ./.. -c ../$FILE.S -o $FILE.o ; \
      objconv -v0 -fcoff -nu $FILE.o ;
done
#
for FILE in \
    loop-x86 loop-x86-sse2 \
    ;
  do gcc -m32 -Wall -DUSE_PREFETCH -DASSEMBLE_FOR_MSC -I ./.. -c ../$FILE.S -o $FILE.p.o ; \
      objconv -v0 -fcoff -nu $FILE.p.o ;
done
