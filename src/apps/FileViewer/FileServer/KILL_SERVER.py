#!/usr/bin/python

### script that kills the FileServer running at the optionally specified port
### this is useful when the FileServer was started by someone else
### but you don't have the permissions to kill it

import xmlrpclib, sys
port = str(8800)
if len(sys.argv) > 1: port = sys.argv[1]
xmlrpclib.ServerProxy("http://localhost:"+port).Quit(1)
