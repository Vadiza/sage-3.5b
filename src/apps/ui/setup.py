############################################################################
#
# SAGE UI - A Graphical User Interface for SAGE
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
# Direct questions, comments etc about SAGE UI to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#        
############################################################################


"""
A distutils script to make a standalone .exe of superdoodle for
Windows platforms.  You can get py2exe from
http://py2exe.sourceforge.net/.  Use this command to build the .exe
and collect the other needed files:

    python setup.py py2exe

A distutils script to make a standalone .app of superdoodle for
Mac OS X.  You can get py2app from http://undefined.org/python/py2app.
Use this command to build the .app and collect the other needed files:

   python setup.py py2app
"""


####  this is used for building on Windows
####  without this we'll get some ugly looking controls

manifest = """
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1"
manifestVersion="1.0">
<assemblyIdentity
    version="0.64.1.0"
    processorArchitecture="x86"
    name="Controls"
    type="win32"
/>
<description>SAGE UI</description>
<dependency>
    <dependentAssembly>
        <assemblyIdentity
            type="win32"
            name="Microsoft.Windows.Common-Controls"
            version="6.0.0.0"
            processorArchitecture="X86"
            publicKeyToken="6595b64144ccf1df"
            language="*"
        />
    </dependentAssembly>
</dependency>
</assembly>
"""
            
#### end windows




VERSION = "3.0"
NAME = "sageui"
DESCRIPTION = "SAGE UI v"+str(VERSION)

from distutils.core import setup, Distribution
import sys, glob


#####    MAC   ######
# RUN WITH: python setup.py py2app --site-packages
#
if sys.platform == 'darwin':
    import py2app
    opts = dict(argv_emulation=True, 
                iconfile="images/sageUIIcon.icns",
                dist_dir="sageui")
    setup(app=['sageui.py'],
	  name = NAME,
	  options=dict(py2app=opts),
          data_files = [(".", ["README","RECENT_CHANGES", "images", "prefs"]),
			("images", glob.glob("images\\*.*")),
                        ("prefs", glob.glob("prefs\\*.*"))],
	  )
	  

#####   WINDOWS   ######
# RUN WITH: python setup.py py2exe -b 1
#

elif sys.platform == 'win32':
    import py2exe
    opts = dict(dist_dir="sageui")
    setup(windows=[{"script":"sageui.py"}],
                    #"other_resources": [(24,1,manifest)]
                   #}],
	  name = NAME,
	  version = str(VERSION),
	  description = DESCRIPTION,
	  options=dict(py2exe=opts),
          data_files = [(".", ["README", "RECENT_CHANGES", "gdiplus.dll", "MSVCP90.dll", "msvcr90.dll"]),
			("images", glob.glob("images\\*.*")),
                        ("prefs", glob.glob("prefs\\*.*"))],
	  )
	  



