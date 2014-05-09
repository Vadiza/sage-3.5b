############################################################################
#
# DIM - A Direct Interaction Manager for SAGE
# Copyright (C) 2013 Electronic Visualization Laboratory,
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
# Direct questions, comments etc about SAGE to sagecommons.org
#
# Author: Luc Renambot
#
############################################################################




# works with oMicron server
# run as: python omicron.py tracker_IP
#


from managerConn import ManagerConnection
import sys, socket, os, time
from threading import Thread
import struct, math


os.chdir(sys.path[0])




# in meters
CAVE_RADIUS = 127.50 * 2.54 / 100

# kinect+pqlabs machine
TCP_IP = '131.193.77.110'
TCP_PORT = 7340
# cave2
#TCP_IP = 'cave2tracker.evl.uic.edu'
#TCP_PORT = 28000

LOCAL_IP = 'traoumad.evl.uic.edu'
DATA_PORT = 10000;
BUFFER_SIZE = 512

omicron = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
omicron.connect((TCP_IP, TCP_PORT))

MESSAGE = "omicron_data_on,%d" % DATA_PORT
print("Sending [%s]" % MESSAGE)
omicron.send(MESSAGE)

udpsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udpsock.bind((LOCAL_IP, DATA_PORT))

"""
                enum ServiceType { 
                        ServiceTypePointer, 
                        ServiceTypeMocap, 
                        ServiceTypeKeyboard, 
                        ServiceTypeController, 
                        ServiceTypeUi, 
                        ServiceTypeGeneric, 
                        ServiceTypeBrain, 
                        ServiceTypeWand, 
                        ServiceTypeSpeech }; 
"""

start_time = None
while True:
    msg, addr = udpsock.recvfrom(BUFFER_SIZE)
    offset = 0
    msglen = len(msg)
    values = {}
    idx = 0
    #print "received message:", msglen, " bytes"
    if offset < msglen:
    	timestamp = struct.unpack_from('<I', msg, offset)
    	offset += 4
	if not start_time:
		start_time = timestamp[0]
		timestamp = 0
	else:
		timestamp = timestamp[0] - start_time
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<i', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<f', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1
	if offset < msglen:
		val = struct.unpack_from('<I', msg, offset)
		values[idx]=val
		offset += 4
		idx += 1

	sourceId    = values[0]
	serviceId   = values[1]
	serviceType = values[2]
	etype       = values[3]
	flags       = values[4]
	posx        = values[5]
	posy        = values[6]
	#print("\t sourceId %d" % sourceId)
	#print("\t serviceId %d" % serviceId)
	#print("\t serviceType %d" % serviceType)
	#print("\t flags %d" % flags)
	#print("\t posx %f" % posx)
	#print("\t posy %f" % posy)
	#print("\t posz %f" % posz)
	#print("\t orw %f" % orw)
	#print("\t orx %f" % orx)
	#print("\t extraDataItems %d" % extraDataItems)
	#print("\t extraDataType %d" % extraDataType)
	#print("\t orz %f" % orz)
	#print("\t ory %f" % ory)
	#print("\t extraDataMask %d" % extraDataMask)


	# ServiceTypePointer
	if serviceType[0] == 0:
		if etype[0] == 3:
			print timestamp, "> Pointer:", sourceId[0], "  type: Update", " pos: ", posx, posy
		if etype[0] == 4:
			print timestamp, "> Pointer:", sourceId[0], "  type: Move", " pos: ", posx, posy
		if etype[0] == 6:
			print timestamp, "> Pointer:", sourceId[0], "  type: Up", " pos: ", posx, posy



s.close()


