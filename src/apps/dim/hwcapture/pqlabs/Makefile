CXXFLAGS=-g -m32
pqlabs: pqlabs.o misc.o Gesture.o clustering.o point.o
	gcc ${CXXFLAGS} -o pqlabs pqlabs.o misc.o Gesture.o clustering.o point.o PQMTClient.a -framework Carbon -framework CoreServices -lstdc++

clean:
	rm *.o pqlabs

pqlabs.o: pqlabs.cpp pqlabs.h topHeader.h

Gesture.o: Gesture.cpp topHeader.h
