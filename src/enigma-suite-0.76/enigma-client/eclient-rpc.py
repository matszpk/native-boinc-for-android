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
import httplib
import urllib2
import xmlrpclib


CHLD_PID=0
LOCKFILE=".eclient.lock"
CRLF="\r\n"
TRIDICT="http://www.bytereef.org/dict/00trigr.cur"
BIDICT="http://www.bytereef.org/dict/00bigr.cur"
TIMEOUT=60
PROXYTIMEOUT=120


class ProxiedTransport(xmlrpclib.Transport):
    """This class is taken from the Python docs, release 2.5a1."""
    def set_proxy(self, proxy):
        self.proxy = proxy
    def make_connection(self, host):
        self.realhost = host
        h = httplib.HTTP(self.proxy)
        return h
    def send_request(self, connection, handler, request_body):
        connection.putrequest("POST", 'http://%s%s' % (self.realhost, handler))
    def send_host(self, connection, host):
        connection.putheader('Host', self.realhost)

class EclientRPC:
    """This class manages a connection to the Enigma XML-RPC Key Server."""

    def usage(self):
        """Display usage message"""
        self.log("usage: eclient-rpc [-r] [-p proxyhost:proxyport] http://rpchost:rpcport")
        sys.exit(1)
 
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

    def filewrite(self, filename, string):
        """Writes 'string' to file."""
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

    def get_chunk(self, server):
        """Attempts to get a chunk for ./enigma to work on.
           Retries until successful.
        """
        done = 0
        while not done:
            reply = (None,)
            try:
                reply = server.want_chunk()
            except xmlrpclib.Fault, fault:
                self.log(''.join(("server message: ", fault.faultString, ": ", str(fault.faultCode))))
                time.sleep(60)
                continue
            except (xmlrpclib.Error, socket.error), e:
                self.log(str(e))
                time.sleep(60)
                continue
            status = reply[0]
            if status == 'TEMP_FAIL':
                time.sleep(60)
                continue
            if status == 'TRY_AGAIN_LATER':
                time.sleep(1800)
                continue
            elif status == 'SUCCESS':
                ticket = reply[1]
                chunk = reply[2]
                ciphertext = reply[3]
                while not self.filewrite('00ciphertext', ciphertext):
                    time.sleep(60)
                while not self.filewrite('00ticket', ticket):
                    time.sleep(60)
                while not self.filewrite('00hc.resume', chunk):
                    time.sleep(60)
                done = 1
            else:
                time.sleep(60)
                continue
    
    def submit_chunk(self, server):
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
            status = None
            try:
                status = server.have_chunk(ticket, chunk)
            except xmlrpclib.Fault, fault:
                self.log(''.join(("server message: ", fault.faultString, ": ", str(fault.faultCode))))
                time.sleep(60)
                continue
            except (xmlrpclib.Error, socket.error), e:
                self.log(str(e))
                time.sleep(60)
                continue
            if status == 'TEMP_FAIL':
                time.sleep(60)
                continue
            elif status == 'TRY_AGAIN_LATER':
                time.sleep(1800)
                continue
            elif status == 'NO_VALID_SCORE':
                self.log("trying to get new dictionaries ...")
                self.persistent_getfiles((TRIDICT, BIDICT), 600)
                self.log("success: got new dictionaries")
                done = 1
            else: # SUCCESS
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
        sys.stderr.write("%s  eclient-rpc: %s\n" % (date, mesg))
        sys.stderr.flush()


""" main """
eclient = EclientRPC()

proxy = None
resultfile = '/dev/null'
optlist = getopt.getopt(sys.argv[1:], "rp:")
for L in optlist[0]:
    if L[0] == "-r":
        resultfile = 'results'
    if L[0] == "-p":
        proxy = L[1]
if len(optlist[1]) != 1:
    eclient.usage()
rpc_url = optlist[1][0]

cmdseq = ('./enigma', '-R', '-o', resultfile, '00trigr.cur', '00bigr.cur', '00ciphertext')

if proxy:
    if hasattr(socket, 'setdefaulttimeout'):
        socket.setdefaulttimeout(PROXYTIMEOUT)
    p = ProxiedTransport()
    p.set_proxy(proxy)
    server = xmlrpclib.Server(rpc_url, transport=p)
else:
    if hasattr(socket, 'setdefaulttimeout'):
        socket.setdefaulttimeout(TIMEOUT)
    server = xmlrpclib.Server(rpc_url)


port = int(rpc_url.split(":")[-1])
if port >= 1024:
    eclient.log("aborting: use enigma-client for ports greater than 1023")
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
        eclient.submit_chunk(server)
        eclient.log("success: submitted results")
        chunk = eclient.filereadlines('00hc.resume')
        if chunk is not None:
            eclient.log(''.join(("best result: ", chunk[1])))
        eclient.log("trying to get workunit ...")
        eclient.get_chunk(server)
        eclient.log("success: got workunit")
    elif retval == 1:
        eclient.log("trying to get workunit ...")
        eclient.get_chunk(server)
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
