#!/usr/bin/python

### script that kills the appLauncher running at the optionally specified port
### this is useful when the appLauncher was started by someone else
### but you don't have the permissions to kill it

import xmlrpclib, sys, socket
port = str(19010)
securePort = str(19011)
if len(sys.argv) > 1: 
    port = sys.argv[1]
try:
    xmlrpclib.ServerProxy("http://localhost:"+port).killLauncher()
except socket.error:
    print "AppLauncher doesn't seem to be running"

print "Done."
