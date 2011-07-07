#/bin/sh
#
# Generate COFF64 objects from GAS x86-64 assembly.
# objconv from http://www.agner.org/optimize.
#
for FILE in \
    mulmod-x86-64 \
    misc-x86-64 \
    powmod-x86-64 \
    btop-x86-64 \
    loop-a64 loop-core2 \
    ;
  do gcc -m64 -Wall -DASSEMBLE_FOR_MSC -I ./.. -c ../$FILE.S -o $FILE.o ; \
      objconv -v0 -fcoff -nu $FILE.o ;
done
#
for FILE in \
    loop-a64 loop-core2 \
    ;
  do gcc -m64 -Wall -DUSE_PREFETCH -DASSEMBLE_FOR_MSC -I ./.. -c ../$FILE.S -o $FILE.p.o ; \
      objconv -v0 -fcoff -nu $FILE.p.o ;
done
