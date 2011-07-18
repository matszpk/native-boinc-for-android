#!/usr/bin/env python
# -*- coding: latin-1 -*-

import sys
import os
import string
import re
import tempfile
import getopt
import math
import locale

LOCALE='de_DE.ISO8859-1'
SGTPATH="."


def usage():
    print "mkdict: usage: mkdict [ -gtbu ] sampletext"
    sys.exit(1)

def translator(frm='', to='', delete='', keep=None):
    if len(to) == 1:
        to = to * len(frm)
    trans = string.maketrans(frm, to)
    if keep is not None:
        allchars = string.maketrans('', '');
        delete = allchars.translate(allchars, keep.translate(allchars, delete))
    def translate(s):
        return s.translate(trans, delete)
    return translate

def multiple_replace(text, subdict):
    rx = re.compile('|'.join(map(re.escape, subdict)))
    def one_xlat(match):
        return subdict[match.group(0)]
    return rx.sub(one_xlat, text)


"""main"""

locale.setlocale(locale.LC_ALL, LOCALE)
subdict = {
   "ä"  : "ae",
   "ö"  : "oe",
   "ü"  : "ue",
   "ß"  : "ss",
   "ê"  : "e",
   "è"  : "e",
   "é"  : "e",
   "ch" : "q"
}

optlist = getopt.getopt(sys.argv[1:], "gstbu")
ngram = None
good_turing = 0
substitute = 0
for L in optlist[0]:
    if L[0] == '-g':
        good_turing = 1
    elif L[0] == '-s':
        substitute = 1
    else:
        if ngram is not None:
            usage()
        ngram = L[0]
if ngram is None:
    ngram = '-t'
if not optlist[1]:
    usage()
else:
    samplefile = optlist[1][0]

sample = open(samplefile, "r").read().lower()
if len(sample) < 100:
    print "Sample text is definitely too short."
    sys.exit(1)
if substitute:
    sample = multiple_replace(sample, subdict)
ascii_only = translator(keep=string.ascii_lowercase)
sample = ascii_only(sample)

nfreq = {}
if ngram == '-t':
    """initialize dictionary"""
    for c1 in string.ascii_lowercase:
        for c2 in string.ascii_lowercase:
            for c3 in string.ascii_lowercase:
                s = ''.join((c1, c2, c3))
                nfreq[s] = 0
    lb = 0; ub = 3
elif ngram == '-b':
    """initialize dictionary"""
    for c1 in string.ascii_lowercase:
        for c2 in string.ascii_lowercase:
            s = ''.join((c1, c2))
            nfreq[s] = 0
    lb = 0; ub = 2
elif ngram == '-u':
    """initialize dictionary"""
    for c in string.ascii_lowercase:
        nfreq[c] = 0
    lb = 0; ub = 1
else:
    usage()

"""build ngram -> freq dictionary"""
while ub <= len(sample):
    nfreq[sample[lb:ub]] += 1
    lb += 1
    ub += 1

"""build list freq, ngram"""
aux1 = [ (nfreq[i], i) for i in nfreq ]
aux1.sort()

if good_turing:
    freqfreq = {}
    """get frequencies of frequencies"""
    for i, k in aux1:
        freqfreq[i] = freqfreq.get(i, 0) + 1

    """build list freq, freqfreq"""
    aux2 = [ (i, freqfreq[i]) for i in freqfreq ]
    aux2.sort()
    
    infile = tempfile.mktemp()
    outfile = tempfile.mktemp()
    ifp = open(infile, "w")
    for i, k in aux2:
        if i != 0:
            ifp.write(''.join((str(i), '\t', str(k), '\n')))
    ifp.close()

    """ SGT calculates gt-smoothed probabilities
        of occurrences of observed frequencies
    """
    command = "PATH=$PATH:"+SGTPATH+" SGT < "+infile+" > "+outfile
    retval = os.system(command) >> 8
    if retval != 0:
        print "mkdict.py: error: SGT failed, try to set SGTPATH"
        sys.exit(1)

    """replace observed frequencies by smoothed probabilities"""
    i = 0
    for line in open(outfile):
        freq, prob = line.split()
        freq = int(freq)
        prob = float(prob)
        if freq == 0 and freqfreq.has_key(0):
            """convert joint probability into individual probability"""
            prob /= freqfreq[0]
        while i < len(aux1) and aux1[i][0] == freq:
            aux1[i] = (prob, aux1[i][1])
            i += 1

    os.remove(infile)
    os.remove(outfile)
else:
    for i, t in enumerate(aux1):
        """turn observed frequencies into probabilities"""
        aux1[i] = (float(t[0]) / len(sample), t[1])
    

aux1.sort()
i = 0
while aux1[i][0] == 0:
    i += 1
m = math.e / aux1[i][0]
"""take logarithms of frequencies"""
for i, t in enumerate(aux1):
    if t[0] != 0:
        aux1[i] = (int(round(10000*math.log(t[0]*m))), t[1])

aux1.reverse()
for i, k in aux1:
    print "%s\t%d" % (k, i)


#
# This file is part of enigma-suite-0.76, which is distributed under the terms
# of the General Public License (GPL), version 2. See doc/COPYING for details.
#
# Copyright (C) 2005 Stefan Krah
# 
