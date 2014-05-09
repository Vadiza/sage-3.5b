/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Copyright (C) 2004-2012 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about SAGE to http://sagecommons.org/
 *
 *****************************************************************************/

-------------------------------------------------------------------------------

		         README file for SAGE v3.x

		written by Byungil(Brent) Jeong, Ratko Jagodic,
                           Luc Renambot
		
-------------------------------------------------------------------------------


0. Differences from v2.1
	(1) Direct interaction with the display is now possible with a joystick
	    connected to a machine where the UI is running
	    (read more about it in ui/README)
	(2) Launching SAGE and all of the components is now greatly simplified
	    through SageLauncher. All of the components can be configured as well.
	(3) SageProxy is now included in the SAGE distribution
	    (for more info check bin/sageProxy/README)
	(4) Bug fixes, minor changes to the applications

	
1. Install libraries
	For SAGE:

	- SDL-1.2.8 or greater
	- readline (runtime and development packages)

	For SageLauncher, SageProxy, AppLauncher, FileServer:

	- python 2.3 or later
	- wxPython 2.6.2 or later
	- numarray or Numeric


2. Setup Environments 
	(1) Set environment variable SAGE_DIRECTORY in your profile
		setenv SAGE_DIRECTORY path_to_sage_directory in .tcshrc
		export SAGE_DIRECTORY=path_to_sage_directory in .bashrc
	(2) Add $SAGE_DIRECTORY/bin to your path
		set path = ($SAGE_DIRECTORY/bin $path) in .tcshrc
		export path=$SAGE_DIRECTORY/bin:$path  in .bashrc
	(3) Add $SAGE_DIRECTORY/lib to LD_LIBRARY_PATH
		setenv LD_LIBRARY_PATH $SAGE_DIRECTORY/lib:$LD_LIBRARY_PATH in .tcshrc
		export LD_LIBRARY_PATH=$SAGE_DIRECTORY/lib:$LD_LIBRARY_PATH in .bashrc	


3. Modify make file
	edit config.mk to fit local installation
	  
4. Compile
	(0) execute './configure' in $SAGE_DIRECTORY
		not a 'real' configure script, mostly staging dependecies for applications
	(1) execute 'make' in $SAGE_DIRECTORY
		(a) with a multi-core machine, try: make -j 4
		(a) with a SSD drive, try: make -j 12
	(2) if you get errors, check include paths and library paths in make files in subdirectories
	(3) execute make install in SAGE_DIRECTORY
		(a) with a multi-core machine, try: make -j 4
		(a) with a SSD drive, try: make -j 12


5. Edit Configuration Files
	(1) go to $SAGE_DIRECTORY/bin
	(2) open "fsManager.conf" and edit following parameters
	  a. fsManager : name and IP address of the machine on which
			Free Space Manager runs. Usually the master node of cluster.
			You can specify two IP addresses here from v1.3.
			The first IP adderss will be used for system components
			(SAIL, SAGE Receiver) and the second IP address will be used
			for UI connections (sent to connection manager).
			If only one IP address is specified, it will be used for both.

	  b. systemPort : port for SAGE system message channel

	  c. uiPort : port for SAGE UI message	channel

	  d. trackPort : port for tracking data (for LambdaTable)

	  e. conManager : the ip address and port number of Connection Manager
		  - Connection manager is an UI server which manages connections among
		    SAGE UIs and fsManagers
		  - If you don't have a connection manager, just leave these fields 
		    as they are

	  f. tileConfiguration : name of tile configuration file	(stdtile-1.conf)	
	  
	  g. receiverSyncPort : port number of sync connection on displaying side
	  h. receiverBufSize : upper bound of receiving buffer size for a pixel stream in MB
	  i. fullScreen : 1 for full screen mode display
	  j. winTime : set the window move/resize latency. If users set this value bigger than 0,
	  		# of window animation steps are dynamically changed to keep this constraint. 
			(under development)
	  k. winStep : set the number of steps in window animation
	  l. rcvNwBufSize : socket buffer size of receivers (SAGE display)
	  m. sendNwBufSize : socket buffer size of senders (SAIL)
	  n. MTU : MTU size for UDP
	  	
	(4) open "stdtile-1.conf" and edit following parameters
	  a. Dimensions : number of columns and rows of tiled display
	  b. Mullions : width(inches) of top, down, left, and right mullions
	  c. Resolution : screen resolution of each tile
	  d. PPI : pixels per inch of each tile
	  e. Machines : number of display nodes which drive tiled display
	  f. for each DisplayNode
	  		Name : name of each display node (not critical)
			IP : ip address of each display node
			Monitors : number of tiles which each node drives
			  (xPos, yPos) of each tile
			  (0,0) is the tile at the lower left corner
	(5) open "tileNodes.list" and list the ip addresses 
		of all your cluster nodes					


6. Test Whether SAGE is Working Properly
	In $SAGE_DIRECTORY/bin
	(1) Execute "fsManager". Tiled display becomes black if it runs correctly.
		Users can specify a configuration file name as command-line argument.
		Otherwise "fsManager.conf" is used.
	(2) Open another terminal and execute "render"
	(3) Open another terminal and execute "fsConsole"
	(4) press TAB key twice. fsConsole commands are listed.
	(5) ? or help command gives you short description for each command
	(6) If you type initial character of a command and press TAB key,
		the command is completed. Then, press TAB key again. You can see
		the description of the command.
	(7) move 0 1000 0 : move the app window


7. Shutdown SAGE
	(1) you can shutdown SAGE using fsConsole command "shutdown"
	(2) if the command doesn't work, execute the script KILL_ALL 


7. Typical Usage - Using SageLauncher to set up, start and stop everything
	You will need all additional dependencies installed - python, wxPython, numarray
	(1) In $SAGE_DIRECTORY/bin type "sage". This will start the SageLauncher which 
	    will help you set up all the SAGE components and get them running easily
	(2) Press START to run SAGE and the checked components 
	    (the default settings should work just fine)
	(3) To shutdown running components (incl SAGE), press STOP.
	(3) Alternatively, check different components that you wish to run with SAGE. 
	    You can also change the settings on each component from the Launcher.
	    

8. Using SAGE Bridge (NOT SUPPORTED ANYMORE)
	(1) SAGE Bridge receives pixel streams from applications and distributes
		to multiple SAGE sessions (a fsManager per session).
	(2) SAGE Bridge is supposed to be executed on high-performance PCs bridging
		rendering clusters and display clusters.
		
	(3) Install SAGE on the SAGE Bridge nodes in the same manner
	(4) Edit sageBridge.conf in $SAGE_DIRECTORY/bin
		- masterIP : the ip address of master node of SAGE Bridge cluster
		- slaveList slaveNum ip1 ip2 ip3 ... : the slave node list of  
			SAGE Bridge cluster (note : multi-node SAGE Bridge is currently 
			unstable. Set the slave number to zero for now. Instead, you
			can run multiple single-node SAGE Bridges independently)
		- streamPort : base port number to be used for the pixel streams between
			apps and SAGE Bridge
		- msgPort : message channel port number
		- syncPort : synchronization channel port number
		- screenRes : debugging window resolution.
			SAGE Bridge can display received pixels for debugging.
			This feature can be on/off by adding/removing BRIDGE_DEBUG_ flag
			in the makefile in $SAGE_DIRECTORY/src
		- rcvNwBufSize, sendNwBufSize, MTU : network parmeters used for
			the pixel streams between apps and SAGE Bridge

	(5) Execute the binary "sageBridge" in $SAGE_DIRECTORY/bin.
		Users can specify a configuration file name as command-line argument.
		Otherwise "sageBridge.conf" is used.
	(6) Execute "fsManager" on display cluster

	(7) Make sure app configuration file includes following items:
		 (appLauncher will generate these automatically if the application
		 was started with a right click from the SAGE UI)
		- bridgeOn true : determine whether this app uses sageBridge or not
		- bridgeIP 192.168.81.1 : master IP of SAGE Bridge
		- bridgePort 42000 : message port of SAGE Bridge
		- fsIP     192.168.81.11 
		- fsPort   20002 : IP and system port of the first fsManager
		
	(8) Launch applications. This is best done through SAGE UI. When the application
		is started with a right-click on the app icon in the UI, it will use
		sageBridge and can then be shared with other SAGE sessions.

	(9) If users want to share this app to another SAGE session through SAGE UI,
	        just drag an application window from one SAGE session onto the tab 
		of the other session and the app will be shared (obviously you must
		be connected to both SAGE sessions for this to work)

		Alternatively, you can share the applications using uiConsole:
		execute "uiConsole" to be connected to the first fsManager
		> uiConsole fsManager-1.conf (config file for the first fsManager)

		Then, execute following command
		
		share appID fsIP2 fsPort2 (appIDs are assigned in execution order
			ex) 0, 1, 2,...  
			fsIP2, fsPort2 : IP and port number of the second fsManager)

		Then, the app window shows up on the second SAGE session.
		
