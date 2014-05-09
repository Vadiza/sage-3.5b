#!/usr/bin/env python
############################################################################
#
# Copyright (C) 2005 Electronic Visualization Laboratory,
# University of Illinois at Chicago
#
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following disclaimer
#    in the documentation and/or other materials provided with the distribution.
#  * Neither the name of the University of Illinois at Chicago nor
#    the names of its contributors may be used to endorse or promote
#    products derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Direct questions, comments etc  to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#
############################################################################


#
# Processes all the images in the passed in directory name and converts
# them to DXT using a imgToDxt
#
# Call with: python dirToDxt.py dirName
#
# Author: Ratko Jagodic
#

import os, sys, os.path

if len(sys.argv) < 2:
    print "\nUsage: python dirToDxt.py directoryName\n"
    sys.exit(0)

count = 0
skippedDirs = []
for root, dirs, files in os.walk(sys.argv[1]):

    # skip dir if we have no write permissions
    if not os.access(root, os.W_OK):
        skippedDirs.append(root)
        print "\n***** No write permissions. Skipping ", root
        continue
    
    for name in files:
        # skip existing dxt files
        if name.endswith(".dxt"):
            continue
        
        count += 1
        filePath = os.path.abspath(os.path.join(root, name))
        print "\n\n#",count, "   -------------- ", filePath, " ---------------"
        cmd = "imgToDxt '%s'" % filePath
        os.system(cmd)


print "\n\n", 10 * "------"
print "Directories skipped due to insufficient write permissions:\n"
for d in skippedDirs:
    print d
print "\n"

