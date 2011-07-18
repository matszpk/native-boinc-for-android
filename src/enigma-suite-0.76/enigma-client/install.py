#!/usr/bin/env python


import sys
import os
import shutil
import time
import warnings
import socket
import httplib
import urllib2
import xmlrpclib
from signal import SIGTERM


CLIENT = None
ARGS = ""
RPC_URL = 'http://keyserver.bytereef.org:443'

PREFIX = os.environ['HOME']
BASEDIR = '.enigma-client'

TIMEOUT = 20
PROXYTIMEOUT = 120

RUNNING_CLIENT_CHECK = 0


BLURB = """

                            Terms of Use
                           ==============

This program is distributed under the General Public License, version 2.
The license can be found in doc/COPYING.

You may install this software on a computer system only if you own
the system or have the permission of the owner.

"""


FGSTART="""#!/bin/sh

cd %(RUNDIR)s

%(RUNDIR)s/%(CLIENT)s %(ARGS)s
"""


ECRUN="""#!/bin/sh

exec </dev/null
exec >/dev/null

cd %(RUNDIR)s

nohup %(RUNDIR)s/%(CLIENT)s %(ARGS)s 2>>logfile &
"""


ECBOOT="""#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

exec </dev/null
exec >/dev/null

cd %(RUNDIR)s

env - PATH=$PATH %(RUNDIR)s/%(CLIENT)s %(ARGS)s 2>>logfile
"""


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


class ConnectTest:
    """This class tests connections to the Enigma Key Server."""

    def __init__(self, host=None, port=0):
        """Constructor. When called without arguments,
           create an unconnected instance.
        """
        self.host = host
        self.port = port
        self.sock = None
        self.file = None
 
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

    def test_get_chunk_rpc(self, server):
        """Tests the connection to the XML-RPC server"""
        reply = (None,)
        try:
            reply = server.want_chunk()
        except xmlrpclib.Fault, fault:
            # print "%s" % fault.faultString
            return 0
        except (xmlrpclib.Error, socket.error), e:
            # print "%s" % e
            return 0
        status = reply[0]
        if status == 'TEMP_FAIL' or status == 'TRY_AGAIN_LATER' or status == 'SUCCESS':
            return 1
        else:
            return 0
    
    def close(self):
        """Close the connection to the enigma key server."""
        if self.file:
            self.file.close()
        self.file = None
        if self.sock:
            self.sock.close()
        self.sock = None
 
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


def mesg(format, *args):
    sys.stdout.write(format % args)
    sys.stdout.flush()

def raw_input_strip(string):
    return raw_input(string).strip()

def get_command_output(command):
    child = os.popen(command)
    data = child.read()
    child.close()
    return data


def find_active_clients():

    oldcwd = os.getcwd()
    cmd = """ps ux | awk '(/enigma/ || /python/) && !/awk/ {print $2}'"""
    candidates  = get_command_output(cmd).split()
    candidates.remove(str(os.getpid()))


    # /proc filesystem facilitates things.
    for pid in candidates:

        procdir = "/proc/" + pid + "/cwd"
        try:
            os.chdir(procdir)
        except OSError:
            continue
        rundir = os.getcwd()
        lockfile = ".eclient.lock"

        if os.path.isfile(lockfile):

            mesg("""

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ The installer found active clients. All clients @
@ have to be stopped first.                       @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
""")
            # Check for /etc/inittab install:
            if os.path.isfile("/etc/inittab"):
                # Uncommented ecboot lines:
                for line in open("/etc/inittab", "r"):
                    if "ecboot" in line and not line.startswith("#"):
                        mesg( "\nFound active ecboot line in /etc/inittab:\n\n%s\n"
                              " ==> Comment out all ecboot lines, then do "
                              "`telinit Q`.\n\n" % line )
                        sys.exit(1)
                # Lines are commented out but user probably forgot `telinit Q`:
                for line in open("/etc/inittab", "r"):
                    if "ecboot" in line and line.startswith("#"):
                        mesg( "\nThis line in /etc/inittab is commented out, but the client is "
                              "still running:\n\n%s\n"
                              " ==> You have to do `telinit Q` to stop the client.\n\n"
                              "If you are sure that the client has not been started by init,\n"
                              "remove the line(s) from /etc/inittab to get rid of this warning."
                              "\n\n" % line )
                        sys.exit(1)

            # Other installs:
            answer = raw_input_strip("\nFound active client using %s. Stop it Y/N [Y]? " % rundir)
            if answer and not answer[0] in "Yy":
                mesg("\n    @@@ Installation aborted. You have to stop the active client first. @@@\n\n")
                sys.exit(1)
            os.kill(int(pid), SIGTERM)
            mesg("\nwaiting for client to stop... ")
            time.sleep(5)
            mesg("done\n\n")

    os.chdir(oldcwd)


def guess_active_clients():

    # Check for enigma first
    cmd = """ps ux | awk '/enigma/ && !/awk/ {print $2}'"""
    candidates  = get_command_output(cmd).split()

    for i in range(len(candidates)):
        candidates[i] = int(candidates[i])

    if candidates != []:
        mesg("""

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ The installer found processes that are probably active clients. @
@ All clients have to be stopped first.                           @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
""")

    for pid in candidates:
        try:
            os.kill(pid, 0)
        except OSError:
            continue
        procstr = get_command_output("ps p %d" % pid)
        answer = raw_input_strip( "\nThis process looks like an active client:\n\n"
                                  "%s \n"
                                  "Stop process Y/N [Y]? " % procstr )
        if not answer or answer[0] in "Yy":
            try:
                os.kill(pid, SIGTERM)
            except OSError:
                pass
            mesg("\nwaiting for process to stop... ")
            time.sleep(5)
            mesg("done\n\n")


    # Check for python processes:
    cmd = """ps ux | awk '/python/ && !/awk/ {print $2}'"""
    candidates  = get_command_output(cmd).split()
    candidates.remove(str(os.getpid()))

    for i in range(len(candidates)):
        candidates[i] = int(candidates[i])

    if candidates != []:
        mesg("""

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ The installer found processes that _might_ be active clients. @
@ All clients have to be stopped first.                         @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
""")

    for pid in candidates:
        procstr = get_command_output("ps p %d" % pid)
        answer = raw_input_strip( "\nThis process _might_ be an active client:\n\n"
                                  "%s \n"
                                  "Stop process Y/N [N]? " % procstr )
        if answer and answer[0] in "Yy":
            try:
                os.kill(pid, SIGTERM)
            except OSError:
                pass
            mesg("\nwaiting for process to stop... ")
            time.sleep(5)
            mesg("done\n")



""" main """

eclient = ConnectTest()
if hasattr(socket, 'setdefaulttimeout'):
    socket.setdefaulttimeout(TIMEOUT)


# Do not run as root.
if os.geteuid() == 0:
    mesg("\n    @@@ Refusing to run as root. @@@\n\n")
    sys.exit(1)


# Legal blurb:
mesg(BLURB)
answer = raw_input_strip("\nI have read the license and I accept Terms of Use. Y/N [N]? ")
if not answer or not answer[0] in "Yy":
    mesg("\n    @@@ Installation aborted. @@@\n\n")
    sys.exit(1)


# If we have /proc, check for running clients now.
if os.path.isdir("/proc"):
    find_active_clients()

guess_active_clients()


mesg("""

#####################################################################
# The installer is going to check the connection to the key server. #
#####################################################################

""" )


# No point in running this script when you are not connected.
answer = raw_input_strip("Are you connected to the Internet Y/N [N]? ")
if not answer or not answer[0] in 'Yy':
    mesg("\nAborting, please run the installer again when you are connected."
         "\n\n")
    sys.exit(1)


# Is bytereef.org reachable at all?
mesg("\ntrying whether the key server is up... ")
eclient.connect("keyserver.bytereef.org", 80)
if eclient.sock is not None:
    eclient.close()
    mesg("yes\n")
else:
    mesg("no\n")
    mesg("\n    @@@ The key server might be down. Try again later. @@@\n\n");
    sys.exit(1)
 

# Test port 65521.
mesg("trying port 65521... ")
eclient.connect("keyserver.bytereef.org", 65521)
if eclient.sock is not None:
    eclient.close()
    CLIENT = "enigma-client.py"
    mesg("success\n\n")
else:
    mesg("no connection\n");
 

# Check for pyexpat.
LACK_PYEXPAT = 0
if CLIENT == None:
    try:
        import pyexpat
    except ImportError:
        LACK_PYEXPAT = 1
        warnings.filterwarnings('ignore', '', DeprecationWarning)


# Test port 443.
if CLIENT == None:
    mesg("trying direct connection to port 443... ")
    server = xmlrpclib.Server(RPC_URL)
    if eclient.test_get_chunk_rpc(server) == 1:
        CLIENT = "eclient-rpc.py"
        mesg("success\n\n")
    else:
        mesg("no connection\n")


# Test port 443 via proxy.
if CLIENT == None:

    if hasattr(socket, 'setdefaulttimeout'):
        socket.setdefaulttimeout(PROXYTIMEOUT)

    mesg("\n    @@@ All direct connection attempts failed. @@@\n\n")

    proxy = raw_input_strip( "Enter a proxy server using the form"
                             " \"127.0.0.1:3128\": ");

    if proxy == "":
        mesg("\n    @@@ Aborting, you have to enter a proxy server. @@@\n")
        sys.exit(1)

    p = ProxiedTransport()
    p.set_proxy(proxy)
    server = xmlrpclib.Server(RPC_URL, transport=p)
    mesg("\ntrying port 443 via proxy at %s... " % proxy)
    if eclient.test_get_chunk_rpc(server) == 1:
        CLIENT = "eclient-rpc.py"
        ARGS = "-p " + proxy + " "
        mesg("success\n\n")
    else:
        mesg("no connection\n\n")
        mesg("All connection attempts failed. Make sure that your connection\n"
             "to the Internet is working and/or check the proxy settings.\n\n")
        sys.exit(1)


# How many cores?
mesg("""
############################################################
# If you have more than one CPU/core, you can run several  #
# instances of the client.                                 #
############################################################

""")

while 1:
    response = raw_input_strip("Enter the number of CPUs/cores you want "
                               "to run the client on [1]: ")
    if response == "":
        response = 1
    try:
        response = int(response)
        if response >= 1 and response <= 32:
            break
    except ValueError:
        pass
    mesg("\n    @@@ Invalid response, try again. @@@\n\n")


# Example of run directory structure:
CORES = response
if CORES == 1:
    mesg( "\nThe installer will create this run directory: %s\n" \
                         % (PREFIX + "/" + BASEDIR) )
else:
    mesg("\nThe installer will create these run directories: \n\n")
    mesg("%s\n" % (PREFIX + "/" + BASEDIR))
    for i in range(1, CORES):
        mesg("%s\n" % (PREFIX + "/" + BASEDIR + str(i+1)))


# Allow choice of prefix for the run directory.
while 1:
    response = raw_input_strip("\nChange the prefix [%s]: " % PREFIX)
    if response == "":
        break
    if not os.path.isabs(response):
        mesg("\n    @@@ Error: %s is not an absolute path. @@@\n" % response)
        continue
    if os.path.exists(response):
        if os.path.isdir(response):
            PREFIX = response
            break
        else:
            mesg("\n @@@ Error: %s exists and is not a directory. @@@\n" % response)
            continue
    else:
        answer = raw_input_strip("\n%s does not exist. Create it Y/N [N]? " % response)
        if answer and answer[0] in 'Yy':
            PREFIX = response
            break


# Allow choice of name for the run directory.
response = raw_input_strip("\nChange the name(s) [%s]: " % BASEDIR)
if response != "":
    BASEDIR = response


PREFIX = os.path.normpath(PREFIX)
BASEDIR = os.path.normpath(BASEDIR)


# Do you really want all of this? :)
mesg("""

##############################
#          Summary           #
##############################

""")

mesg("Client:\t%s\n" % CLIENT)

if CORES == 1:
    mesg( "\nThe installer will create/use this run directory: %s\n" \
                         % (PREFIX + "/" + BASEDIR) )
else:
    mesg("\nThe installer will create/use these run directories: \n\n")
    mesg("%s\n" % (PREFIX + "/" + BASEDIR))
    for i in range(1, CORES):
        mesg("%s\n" % (PREFIX + "/" + BASEDIR + str(i+1)))

answer = raw_input_strip("\nAccept Y/N [N]? ")
if not answer or not answer[0] in "Yy":
    mesg("\n    @@@ Installation aborted. @@@\n\n")
    sys.exit(1)


# Put the command line together.
if CLIENT == "enigma-client.py":
    ARGS = "keyserver.bytereef.org 65521"
elif CLIENT == "eclient-rpc.py":
    ARGS = ARGS + RPC_URL


# Create first run directory.
RUNDIR = PREFIX + "/" + BASEDIR
if not os.path.exists(RUNDIR):
    os.makedirs(RUNDIR)

# If there are files from a previous installation, remove them.
for filename in ( "enigma", "enigma-client.py", "eclient-rpc.py", "ecrun",
                  "ecboot", "fgstart" ):
    try:
        os.remove(RUNDIR + "/" + filename)
    except OSError:
        pass

# Copy the new files.
shutil.copy("../enigma", RUNDIR)
shutil.copy(CLIENT, RUNDIR)

filename = RUNDIR + "/" + "fgstart"
s = FGSTART % {"RUNDIR": RUNDIR, "CLIENT": CLIENT, "ARGS": ARGS}
eclient.filewrite(filename, s)
os.chmod(filename, 0755)

filename = RUNDIR + "/" + "ecrun"
s = ECRUN % {"RUNDIR": RUNDIR, "CLIENT": CLIENT, "ARGS": ARGS}
eclient.filewrite(filename, s)
os.chmod(filename, 0755)

filename = RUNDIR + "/" + "ecboot"
s = ECBOOT % {"RUNDIR": RUNDIR, "CLIENT": CLIENT, "ARGS": ARGS}
eclient.filewrite(filename, s)
os.chmod(filename, 0755)


# Create and populate additional run directories as needed.
for i in range(1, CORES):

    # Create additional run directory.
    RUNDIR = PREFIX + "/" + BASEDIR + str(i+1)
    if not os.path.exists(RUNDIR):
        os.makedirs(RUNDIR)

    # If there are files from a previous installation, remove them.
    for filename in ( "enigma", "enigma-client.py", "eclient-rpc.py", "ecrun",
                      "ecboot", "fgstart" ):
        try:
            os.remove(RUNDIR + "/" + filename)
        except OSError:
            pass

    # Copy the new files.
    shutil.copy("../enigma", RUNDIR)
    shutil.copy(CLIENT, RUNDIR)

    filename = RUNDIR + "/" + "fgstart"
    s = FGSTART % {"RUNDIR": RUNDIR, "CLIENT": CLIENT, "ARGS": ARGS}
    eclient.filewrite(filename, s)
    os.chmod(filename, 0755)

    filename = RUNDIR + "/" + "ecrun"
    s = ECRUN % {"RUNDIR": RUNDIR, "CLIENT": CLIENT, "ARGS": ARGS}
    eclient.filewrite(filename, s)
    os.chmod(filename, 0755)

    filename = RUNDIR + "/" + "ecboot"
    s = ECBOOT % {"RUNDIR": RUNDIR, "CLIENT": CLIENT, "ARGS": ARGS}
    eclient.filewrite(filename, s)
    os.chmod(filename, 0755)


mesg("\n\n    $$$ Created run directories. $$$\n\n")


# How many cores?
mesg("""
######################################################
# If you want to run the client automatically in the #
# background (recommended), you have two options.    #
######################################################

""")


# /etc/inittab
mesg("Option 1:\n")
mesg("=========\n\n")
mesg("# Add this to /etc/inittab:\n\n")

# First directory
FULLPATH = PREFIX + "/" + BASEDIR + "/" + "ecboot"
mesg("EC:2345:respawn:/bin/su - %s -c %s\n", os.environ['USER'], FULLPATH)

# Additional directories
for i in range(1, CORES):

    FULLPATH = PREFIX + "/" + BASEDIR + str(i+1) + "/" + "ecboot"
    mesg( "EC%s:2345:respawn:/bin/su - %s -c %s\n", str(i+1),
          os.environ['USER'], FULLPATH )


# crontab
mesg("\n\nOption 2:\n")
mesg("=========\n\n")
mesg("# Attention: The crontab syntax is for the Vixie cron.\n")
mesg("# Add this to your crontab (crontab -e):\n\n")

# First directory
FULLPATH = PREFIX + "/" + BASEDIR + "/" + "ecrun"
mesg("@reboot %s\n", FULLPATH)

# Additional directories
for i in range(1, CORES):

    FULLPATH = PREFIX + "/" + BASEDIR + str(i+1) + "/" +  "ecrun"
    mesg("@reboot %s\n", FULLPATH)

mesg("\n\n")


# Missing pyexpat.so warning:
if LACK_PYEXPAT:
    mesg("""
    !!!  python: DeprecationWarning: Could not find pyexpat.  !!!
    !!!                                                       !!!
    !!!  Depending on your distribution, consider installing  !!!
    !!!  python-expat or python-xml.                          !!!
    !!!  Otherwise you will have annoying but harmless        !!!
    !!!  deprecation warnings in your logs.                   !!!


""")


#
# This file is part of enigma-suite-0.76, which is distributed under the terms
# of the General Public License (GPL), version 2. See doc/COPYING for details.
#
# Copyright (C) 2006 Stefan Krah
#
