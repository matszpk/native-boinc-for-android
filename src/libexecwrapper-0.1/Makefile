###################
# Makefile
# Author: Mateusz Szpakowski
########################

DEFS=-DDEBUG=1 -DTESTING=1 -DSLEEPING=1
#DEFS=-DTESTING=1
#DEFS=

LIBEW = libexecwrapper.so

all: $(LIBEW)

clean:
	rm -f *.o $(LIBEW)

LIBEWSRCS = execinit.c execwrapper.c
LIBEWOBJS = execinit.o execwrapper.o


$(LIBEW): $(LIBEWOBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -shared -o $@ $^

HDRS = config.h

SRCS = $(LIBEWSRCS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFS) -c -o $@ $<

DEPS = make.deps

make.deps: $(SRCS) $(HDRS)
	$(CC) -std=gnu99 $(CPPFLAGS) $(INCDIRS) $(DEFS) -MM $(SRCS) > make.deps

include $(DEPS)
