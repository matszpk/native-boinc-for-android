# Makefile for enigma-suite-0.76

VPATH = .:tools

default: enigma SGT

charmap.o: \
compile charmap.c charmap.h
	./compile charmap.c

cipher.o: \
compile cipher.c global.h charmap.h key.h cipher.h
	./compile cipher.c

ciphertext.o: \
compile ciphertext.c error.h ciphertext.h
	./compile ciphertext.c

compile: \
warn-auto.sh conf-cc
	( cat warn-auto.sh; \
	echo exec "`head -1 conf-cc`" '-c $${1+"$$@"}' \
	) > compile
	chmod 755 compile

date.o: \
compile date.c date.h
	./compile date.c

dict.o: \
compile dict.c charmap.h error.h dict.h
	./compile dict.c

display.o: \
compile display.c display.h
	./compile display.c

enigma.o: \
compile enigma.c global.h charmap.h cipher.h ciphertext.h dict.h \
display.h error.h hillclimb.h ic.h input.h key.h result.h \
resume_in.h resume_out.h scan.h
	./compile enigma.c

enigma: \
load enigma.o charmap.o cipher.o ciphertext.o date.o dict.o \
display.o error.o hillclimb.o ic.o input.o key.o result.o \
resume_in.o resume_out.o scan_int.o score.o stecker.o
	./load enigma charmap.o cipher.o ciphertext.o date.o dict.o \
	display.o error.o hillclimb.o ic.o input.o key.o result.o \
	resume_in.o resume_out.o scan_int.o score.o stecker.o -lm

error.o: \
compile error.c error.h date.h
	./compile error.c

hillclimb.o: \
compile hillclimb.c cipher.h dict.h error.h global.h key.h \
result.h resume_out.h score.h stecker.h state.h hillclimb.h
	./compile hillclimb.c
	
ic.o: \
compile ic.c cipher.h global.h hillclimb.h key.h ic.h
	./compile ic.c

input.o: \
compile input.c charmap.h error.h global.h key.h stecker.h input.h
	./compile input.c

key.o: \
compile key.c global.h key.h
	./compile key.c

load: \
warn-auto.sh conf-ld
	( cat warn-auto.sh; \
	echo 'main="$$1"; shift'; \
	echo exec "`head -1 conf-ld`" \
	'-o "$$main" "$$main".o $${1+"$$@"}' \
	) > load
	chmod 755 load

result.o: \
compile result.c charmap.h date.h error.h global.h key.h result.h
	./compile result.c

resume_in.o: \
compile resume_in.c error.h global.h input.h key.h resume_in.h \
scan.h stecker.h
	./compile resume_in.c

resume_out.o: \
compile resume_out.c charmap.h global.h key.h state.h resume_out.h
	./compile resume_out.c

scan_int.o: \
compile scan_int.c scan.h
	./compile scan_int.c

score.o: \
compile score.c cipher.h key.h score.h
	./compile score.c

stecker.o: \
compile stecker.c key.h stecker.h
	./compile stecker.c


# =============
#  tools/SGT.c
# =============

tools/SGT: \
tools/SGT.o load
	./load tools/SGT -lm

tools/SGT.o: \
SGT.c compile
	./compile $< -o $@


clean: FORCE
	rm -f *.o  tools/*.o

FORCE:
