CPP=c++
BUILD_FLAGS=-g -O3 -Wall -DPIPE_NOT_EFD -Wno-deprecated
ARCH=x86-64

all:	ufHTTPServer

UF.o:	UF.C UF.H
	$(CPP) $(BUILD_FLAGS) -c -o UF.o UF.C -march=$(ARCH) 

UFIO.o:	UFIO.C UFIO.H
	$(CPP) $(BUILD_FLAGS) -c -o UFIO.o UFIO.C -march=$(ARCH)

UFStatSystem.o: UFStatSystem.C UFStatSystem.H
	$(CPP) $(BUILD_FLAGS) -c -o UFStatSystem.o UFStatSystem.C -march=$(ARCH)

UFServer.o: UFServer.C UFServer.H
	$(CPP) $(BUILD_FLAGS) -c -o UFServer.o UFServer.C -march=$(ARCH)

ufHTTPServer.o:	ufHTTPServer.C UF.o UFIO.o UFStatSystem.o UFServer.o
	$(CPP) $(BUILD_FLAGS) -c -o ufHTTPServer.o ufHTTPServer.C -march=$(ARCH)

ufHTTPServer:	ufHTTPServer.o
	$(CPP) $(BUILD_FLAGS) -o ufHTTPServer UF.o UFIO.o UFStatSystem.o UFServer.o ufHTTPServer.o -lpthread -march=$(ARCH)

clean: 
	rm *.o ufHTTPServer
