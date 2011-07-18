#!/usr/bin/env python

import sys

for line in sys.stdin:
    tri, log = line.split()
    log = float(log)
    print "%s\t%d" % (tri, int(round(log*10000)))
