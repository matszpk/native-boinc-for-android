#!/usr/bin/env python

import fcntl
import os
import popen2
import signal
import sys
import getopt
import re
import time
import socket
import urllib2


CHLD_PID=0
LOCKFILE=".eclient.lock"
CRLF="\r\n"
TRIDICT="http://www.bytereef.org/dict/00trigr.cur"
BIDICT="http://www.bytereef.org/dict/00bigr.cur"
TIMEOUT=60

class Eclient:
    """This class manages a connection to the Enigma Key Server."""

    def __init__(self, host=None, port=0):
        """Constructor. When called without arguments,
           create an unconnected instance.
        """
        self.host = host
        self.port = port
        self.sock = None
        self.file = None

    def usage(self):
        """Display usage message"""
        self.log("usage: enigma-client [-r] host port")
        sys.exit(1)
 
    def connect(self, host=None, port=0):
        """ Create socket and connect to a host on a given port."""
        self.sock = None
        self.host = host
        self.port = port
        try:
            for res in socket.getaddrinfo(host, port, 0, socket.SOCK_STREAM):
                af, socktype, proto, canonname, sa = res
                try:
                    self.sock = socket.socket(af, socktype, proto)
                    self.sock.connect(sa)
                except socket.error:
                    if self.sock:
                        self.sock.close()
                    self.sock = None
                    continue
                break
        except socket.gaierror:
            return

    def persistent_connect(self, host=None, port=0, interval=1800):
        """Connect. Retry until successful."""
        self.sock = None
        while not self.sock:
            self.connect(host, port);
            if not self.sock:
                self.log(''.join(("unable to connect to ", host, ":", str(port))))
                time.sleep(interval)

    def persistent_getfiles(self, urls=[], interval=600):
        """Get files specified by urls[]. Retry until successful."""
        non_fatal = (IOError, urllib2.HTTPError)
        for u in urls:
            while 1:
                try:
                    sock = urllib2.urlopen(u)
                except non_fatal, err:
                    self.log(err)
                    time.sleep(interval)
                else:
                    s = sock.read()
                    sock.close()
                    name = os.path.basename(u)
                    self.filewrite(name , s)
                    break

    def close(self):
        """Close the connection to the Enigma Key Server."""
        if self.file:
            self.file.close()
        self.file = None
        if self.sock:
            self.sock.close()
        self.sock = None

    def sendline(self, string):
        """Send 'string' postfixed with CRLF to the server."""
        string = string.rstrip()
        string = '%s%s' % (string, CRLF)
        try:
            self.sock.sendall(string)
        except socket.error:
            self.close()
            return 0
        return 1
 
    def getline(self):
        """Attempts to read a single line from the network buffer.
           Replaces CRLF with \n. Returns the line or 'None' on error.
        """
        if self.file is None:
            self.file = self.sock.makefile('rb')
        try:
            line = self.file.readline()
        except socket.error:
            self.close()
            return None
        if line == '':
            self.close()
            return None
        line = re.sub(CRLF, "\n", line)
        return line
 
    def filewrite(self, filename, string):
        """Writes 'string' to filename."""
        try:
            f = open(filename, 'w')
        except EnvironmentError:
            return 0
        try:
            f.write(string)
        except EnvironmentError:
            f.close()
            return 0
        f.close()
        return 1

    def fileread(self, filename):
        """Returns contents of filename in a string"""
        try:
            f = open(filename, 'r')
        except EnvironmentError:
            return None
        try:
            s = f.read()
        except EnvironmentError:
            f.close()
            return None
        f.close()
        return s

    def filereadlines(self, filename):
        """Returns contents of filename in a list of lines"""
        try:
            f = open(filename, 'r')
        except EnvironmentError:
            return None
        try:
            list = f.readlines()
        except EnvironmentError:
            f.close()
            return None
        f.close()
        return list

    def get_chunk(self, host, port):
        """Attempts to get a chunk for ./enigma to work on.
           Retries until successful.
        """
        done = 0
        while not done:
            self.persistent_connect(host, port, 600)
            if not self.sendline('WANT_CHUNK'):
                time.sleep(10)
                continue
            reply = self.getline()        
            if reply == 'TRY_AGAIN_LATER\n':
                self.close()
                time.sleep(1800)
                continue
            elif reply == 'OK\n':
                i = 0
                c = 0
                data = [None]*4
                while i < 4:
                    data[i] = self.getline()
                    if data[i] is None:
                        c = 1
                        break
                    i += 1
                if c:
                    self.close()
                    time.sleep(10)
                    continue
                ticket = data[0]
                chunk = ''.join([data[1], data[2]])
                ciphertext = data[3]
    
                self.close()
                while not self.filewrite('00ciphertext', ciphertext):
                    time.sleep(60)
                while not self.filewrite('00ticket', ticket):
                    time.sleep(60)
                while not self.filewrite('00hc.resume', chunk):
                    time.sleep(60)
                done = 1
            else:
                self.close()
                time.sleep(10)
                continue
 
    def submit_chunk(self, host, port):
        """Attempts to submit a chunk that has been processed by ./enigma
           Retries until successful.
        """
        ticket = self.fileread('00ticket')
        if ticket is None:
            return
        chunk = self.filereadlines('00hc.resume')
        if chunk is None:
            return
        done = 0
        while not done:
            self.persistent_connect(host, port, 600)
            if not self.sendline('HAVE_CHUNK'):
                time.sleep(10)
                continue
            reply = self.getline()        
            if reply != 'SEND_TICKET\n':
                self.close()
                time.sleep(10)
                continue
            if not self.sendline(ticket):
                time.sleep(10)
                continue
            reply = self.getline()
            if reply != 'SEND_CHUNK\n':
                self.close()
                time.sleep(10)
                continue;
            if not self.sendline(chunk[0]):
                time.sleep(10)
                continue
            if not self.sendline(chunk[1]):
                time.sleep(10)
                continue
            reply = self.getline()
            if reply == 'TRY_AGAIN_LATER\n':
                self.close()
                time.sleep(1800)
                continue
            elif reply == 'NO_VALID_SCORE\n':
                self.close()
                self.log("trying to get new dictionaries ...")
                self.persistent_getfiles((TRIDICT, BIDICT), 600)
                self.log("success: got new dictionaries")
                done = 1
            else: # SUCCESS
                self.close()
                done = 1

    def lock(self, file, flags):
        fcntl.flock(file.fileno(), flags)

    def cleanup(self, signum, frame):
        try:
            os.kill(CHLD_PID, signal.SIGTERM);
        except OSError:
            pass
        sys.exit(111)

    def install_sighandlers(self):
        signal.signal(signal.SIGTERM, self.cleanup)
        signal.signal(signal.SIGINT, self.cleanup)
        signal.signal(signal.SIGQUIT, self.cleanup)

    def log(self, mesg):
        date = time.strftime("%Y-%m-%d %H:%M:%S")
        sys.stderr.write("%s  enigma-client: %s\n" % (date, mesg))
        sys.stderr.flush()


""" main """
eclient = Eclient()

resultfile = '/dev/null'
optlist = getopt.getopt(sys.argv[1:], "r")
for L in optlist[0]:
    if L[0] == "-r":
        resultfile = 'results'
if len(optlist[1]) != 2:
    eclient.usage()
host = optlist[1][0]
port = int(optlist[1][1])

cmdseq = ('./enigma', '-R', '-o', resultfile, '00trigr.cur', '00bigr.cur', '00ciphertext')
if hasattr(socket, 'setdefaulttimeout'):
    socket.setdefaulttimeout(TIMEOUT)


if port == 65500:
    eclient.log("aborting: 65000 is the old port number")
    sys.exit(1)

if port < 1024:
    eclient.log("aborting: use eclient-rpc for ports less than 1024")
    sys.exit(1)

if not os.path.isfile('enigma'):
    eclient.log("error: could not find enigma")
    sys.exit(1)

if not os.path.isfile('00trigr.cur'):
    eclient.log("trying to get 00trigr.cur ...")
    eclient.persistent_getfiles((TRIDICT,), 600)
    eclient.log("success: got 00trigr.cur")

if not os.path.isfile('00bigr.cur'):
    eclient.log("trying to get 00bigr.cur ...")
    eclient.persistent_getfiles((BIDICT,), 600)
    eclient.log("success: got 00bigr.cur")


lockfile = open(LOCKFILE, "a")
try:
    eclient.lock(lockfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
except IOError:
    eclient.log("error: another_enigma-client_process_is_already_using_this_directory")
    sys.exit(1)


eclient.install_sighandlers()
os.nice(19)


while 1:
    proc = popen2.Popen3(cmdseq)
    CHLD_PID = proc.pid
    retval = proc.wait() >> 8
    if retval == 0:
        eclient.log("submitting results ...")
        eclient.submit_chunk(host, port)
        eclient.log("success: submitted results")
        chunk = eclient.filereadlines('00hc.resume')
        if chunk is not None:
            eclient.log(''.join(("best result: ", chunk[1])))
        eclient.log("trying to get workunit ...")
        eclient.get_chunk(host, port)
        eclient.log("success: got workunit")
    elif retval == 1:
        eclient.log("trying to get workunit ...")
        eclient.get_chunk(host, port)
        eclient.log("success: got workunit")
        time.sleep(10)
    else:
        """./enigma has caught a signal"""
        sys.exit(retval)


#
# This file is part of enigma-suite-0.76, which is distributed under the terms
# of the General Public License (GPL), version 2. See doc/COPYING for details.
#
# Copyright (C) 2005 Stefan Krah
# 
