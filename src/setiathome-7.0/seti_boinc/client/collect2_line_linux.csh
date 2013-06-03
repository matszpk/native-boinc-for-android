#! /bin/csh

# Run this with the desired version number as the only operand.  Ex:
# collect2_line 2.27

/usr/local/lib/gcc-lib/i686-pc-linux-gnu/3.2.2/collect2 -m elf_i386 -dynamic-linker /lib/ld-linux.so.2 -o setiathome_$1_i686-pc-linux-gnu /usr/lib/crt1.o /usr/lib/crti.o /usr/local/lib/gcc-lib/i686-pc-linux-gnu/3.2.2/crtbegin.o -L. -L../../boinc/lib -L/usr/local/lib/gcc-lib/i686-pc-linux-gnu/3.2.2 -L/usr/local/lib/gcc-lib/i686-pc-linux-gnu/3.2.2/../../.. main.o analyzeFuncs.o analyzeReport.o analyzePoT.o pulsefind.o gaussfit.o lcgamm.o malloc_a.o seti.o seti_header.o timecvt.o s_util.o version.o worker.o chirpfft.o spike.o progress.o ../db/schema_master_client.o ../db/sqlrow_client.o ../db/sqlblob.o ../db/xml_util.o -looura  -Bstatic -lstdc++ -Bdynamic -lz -lnsl -ldl -lboinc  -Bstatic -lstdc++ -Bdynamic -lm -lgcc_s -lgcc -lc -lgcc_s -lgcc /usr/local/lib/gcc-lib/i686-pc-linux-gnu/3.2.2/crtend.o /usr/lib/crtn.o
